/**
 * Render Pipeline C++
 *
 * Copyright (c) 2014-2016 tobspr <tobias.springer1@gmail.com>
 * Copyright (c) 2016-2017 Center of Human-centered Interaction for Coexistence.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "render_pipeline/rpcore/effect.hpp"

#include <regex>

#include <shader.h>
#include <filename.h>
#include <graphicsWindow.h>

#include <fmt/format.h>

#include <boost/algorithm/string.hpp>

#include "render_pipeline/rppanda/stdpy/file.hpp"
#include "render_pipeline/rppanda/showbase/showbase.hpp"
#include "render_pipeline/rpcore/render_pipeline.hpp"
#include "render_pipeline/rpcore/globals.hpp"
#include "render_pipeline/rpcore/loader.hpp"
#include "render_pipeline/rpcore/stage_manager.hpp"

#include "rplibs/yaml.hpp"

namespace rpcore {

class Effect::Impl
{
public:
    using InjectionType = std::unordered_map<std::string, std::vector<std::string>>;

    static std::string generate_hash(const Filename& filename, const OptionType& options);

public:
    /**
     * Configuration options which can be set per effect instance.These control
     * which features are available in the effect, and which passes to render.
     */
    static OptionType default_options_;

    /**
     * All supported render passes, should match the available passes in the
     * TagStateManager class.
     */
    static std::vector<PassType> passes_;

    /**
     * Effects are cached based on their source filename and options, this is
     * the cache where compiled are effects stored.
     */
    static std::unordered_map<std::string, std::shared_ptr<Effect>> global_cache_;

    /**
     * Global counter to store the amount of generated effects, used to create
     * a unique id used for writing temporary files.
     */
    static int effect_id_;

public:
    /**
     * Constructs an effect name from a filename, this is used for writing
     * out temporary files.
     */
    std::string convert_filename_to_name(const Filename& filepath);

    /** Internal method to construct the effect from a yaml object. */
    void parse_content(RenderPipeline& pipeline, Effect& self, YAML::Node& parsed_yaml);

    /**
     * Parses a fragment template. This just finds the default template
     * for the shader, and redirects that to construct_shader_from_data.
     */
    void parse_shader_template(RenderPipeline& pipeline, Effect& self, const PassType& pass_id_multiview, const std::string& stage,
        YAML::Node& data);

    /** Constructs a shader from a given dataset. */
    std::string construct_shader_from_data(Effect& self, const std::string& pass_id, const std::string& stage,
        const Filename& template_src, YAML::Node& data);

    /**
     * Generates a compiled shader object from a given shader
     * source location and code injection definitions.
     */
    std::string process_shader_template(Effect& self, const Filename& template_src,
        const std::string& cache_key, InjectionType& injections);

public:
    int this_effect_id_ = effect_id_;
    Filename filename_;
    std::string effect_name_;
    std::string effect_hash_;
    OptionType options_;
    std::unordered_map<std::string, std::string> generated_shader_paths_;

    std::unordered_map<std::string, PT(Shader)> shader_objs_;
};

Effect::OptionType Effect::Impl::default_options_ =
{
    {pass_option_prefix + std::string("gbuffer"), true},
    {pass_option_prefix + std::string("shadow"), true},
    {pass_option_prefix + std::string("voxelize"), true},
    {pass_option_prefix + std::string("envmap"), true},
    {pass_option_prefix + std::string("forward"), false},
    {"alpha_testing", true},
    {"normal_mapping", true},
    {"parallax_mapping", false},
};

std::vector<Effect::PassType> Effect::Impl::passes_ = {{"gbuffer", true}, {"shadow", false}, {"voxelize", false}, {"envmap", false}, {"forward", true}};

std::unordered_map<std::string, std::shared_ptr<Effect>> Effect::Impl::global_cache_;
int Effect::Impl::effect_id_ = 0;

std::string Effect::Impl::generate_hash(const Filename& filename, const OptionType& options)
{
    // Set all options which are not present in the dict to its defaults
    OptionType opt = default_options_;
    for (auto&& pair: opt)
    {
        auto found = options.find(pair.first);
        if (found != options.end())
            pair.second = found->second;
    }

    // Hash filename, make sure it has the right format and also resolve
    // it to an absolute path, to make sure that relative paths are cached
    // correctly(otherwise, specifying a different path to the same file
    // will cause a cache miss)
    Filename fname = filename;
    fname.make_absolute();
    const std::string& file_hash = std::to_string(fname.get_hash());

    // Hash the options, that is, sort the keys to make sure the values
    // are always in the same order, and then convert the flags to strings using
    // '1' for a set flag, and '0' for a unset flag
    std::string options_hash;
    options_hash.reserve(opt.size());
    for (const auto& pair: opt)
        options_hash += pair.second ? "1" : "0";

    return file_hash + "-" + options_hash;
}

std::string Effect::Impl::convert_filename_to_name(const Filename& filepath)
{
    std::string filename = filepath.get_basename_wo_extension();
    boost::replace_all(filename, "/", "_");
    boost::replace_all(filename, "\\", "_");
    boost::replace_all(filename, ".", "_");
    return filename;
}

void Effect::Impl::parse_content(RenderPipeline& pipeline, Effect& self, YAML::Node& parsed_yaml)
{
    YAML::Node vtx_data = parsed_yaml["vertex"];
    YAML::Node geom_data = parsed_yaml["geometry"];
    YAML::Node frag_data = parsed_yaml["fragment"];

    for (const auto& pass: passes_)
    {
        parse_shader_template(pipeline, self, pass, "vertex", vtx_data);
        parse_shader_template(pipeline, self, pass, "geometry", geom_data);
        parse_shader_template(pipeline, self, pass, "fragment", frag_data);
    }
}

void Effect::Impl::parse_shader_template(RenderPipeline& pipeline, Effect& self, const PassType& pass, const std::string& stage, YAML::Node& data)
{
    const std::string& pass_id = pass.id;
    bool stereo_mode = pipeline.is_stereo_mode() && pass.stereo_flag;

    Filename template_src;
    if (stage == "fragment")
    {
        if (pass.template_fragment.empty())
            template_src = "/$$rp/shader/templates/" + pass_id + ".frag.glsl";
        else
            template_src = pass.template_fragment;
    }
    else if (stage == "vertex")
    {
        if (pass.template_vertex.empty())
        {
            // Using a shared vertex shader
            if (stereo_mode)
                template_src = "/$$rp/shader/templates/vertex_stereo.vert.glsl";
            else
                template_src = "/$$rp/shader/templates/vertex.vert.glsl";
        }
        else
        {
            template_src = pass.template_vertex;
        }
    }
    else if (stage == "geometry")
    {
        if (pass.template_geometry.empty())
        {
            // for stereo, add geometry shader except that NVIDIA single pass stereo exists.
            if (stereo_mode && pipeline.get_stage_mgr()->get_defines().at("NVIDIA_STEREO_VIEW") == "0")
            {
                template_src = "/$$rp/shader/templates/vertex_stereo.geom.glsl";
            }
        }
        else
        {
            template_src = pass.template_geometry;
        }
    }

    if (template_src.empty())
        return;

    if (!rppanda::exists(template_src))
    {
        self.error(fmt::format("Template file does not exist: {}", template_src.to_os_generic()));
        return;
    }

    const std::string& shader_path = construct_shader_from_data(self, pass_id, stage, template_src, data);
    generated_shader_paths_[stage + "-" + pass_id] = shader_path;
}

std::string Effect::Impl::construct_shader_from_data(Effect& self, const std::string& pass_id, const std::string& stage,
    const Filename& template_src, YAML::Node& data)
{
    InjectionType injects ={{std::string("defines"), {}}};

    for (const auto& key_val: options_)
    {
        const std::string& val_str = key_val.second ? "1" : "0";

        injects["defines"].push_back(std::string("#define OPT_" +
            boost::algorithm::to_upper_copy(key_val.first) + " " + val_str));
    }

    injects["defines"].push_back(std::string("#define IN_") + boost::algorithm::to_upper_copy(stage) + "_SHADER 1");
    injects["defines"].push_back(std::string("#define IN_") + boost::algorithm::to_upper_copy(pass_id) + "_SHADER 1");
    injects["defines"].push_back(std::string("#define IN_RENDERING_PASS 1"));

    // Parse dependencies
    if (data.IsDefined() && data["dependencies"])
    {
        for (const auto& dependency: data["dependencies"])
        {
            const std::string& include_str = std::string("#pragma include \"") + dependency.as<std::string>() + "\"";
            injects["includes"].push_back(include_str);
        }
        data.remove("dependencies");
    }

    // Append aditional injects
    for (const auto& node: data)
    {
        const std::string key(node.first.as<std::string>());

        if (node.second.IsNull())
        {
            self.warn(std::string("Empty insertion: '") + key + "'");
            continue;
        }

        const std::string val(node.second.as<std::string>());
        if (!node.second.IsScalar())
        {
            self.warn("Invalid syntax, you used a list but you should have used a string:");
            self.warn(val);
            continue;
        }

        std::regex newline_re("\\n");
        std::vector<std::string> parsed_val(std::sregex_token_iterator(val.begin(), val.end(), newline_re, -1), std::sregex_token_iterator());
        injects[key].insert(injects[key].end(), parsed_val.begin(), parsed_val.end());
    }

    const std::string& cache_key = effect_name_ + "@" + stage + "-" + pass_id + "@" + effect_hash_;
    return process_shader_template(self, template_src, cache_key, injects);
}

std::string Effect::Impl::process_shader_template(Effect& self, const Filename& template_src, const std::string& cache_key,
    InjectionType& injections)
{
    std::vector<std::string> shader_lines;

    try
    {
        auto file = rppanda::open_read_file(template_src, true);

        do
        {
            shader_lines.push_back("");
        } while (std::getline(*file, shader_lines.back()));
    }
    catch (const std::exception& err)
    {
        self.error(std::string("Error reading shader template: ") + err.what());
    }

    std::vector<std::string> parsed_lines ={std::string("\n\n")};
    parsed_lines.push_back("/* Compiled Shader Template");
    parsed_lines.push_back(std::string(" * generated from: '") + template_src.to_os_generic() + "'");
    parsed_lines.push_back(std::string(" * cache key: '") + cache_key + "'");
    parsed_lines.push_back(" *");
    parsed_lines.push_back(" * !!! Autogenerated, do not edit! Your changes will be lost. !!!");
    parsed_lines.push_back(" */\n\n");

    // Store whether we are in the main function already - we need this
    // to properly insert scoped code blocks
    bool in_main = false;

    for (const auto& line: shader_lines)
    {
        const std::string stripped_line(boost::to_lower_copy(boost::trim_copy(line)));

        // Check if we are already in the main function
        if (stripped_line.find("void main()") != std::string::npos)
            in_main = true;

        // Check if the current line is a hook
        if (!stripped_line.empty() && stripped_line.front() == '%' && stripped_line.back() == '%')
        {
            // If the line is a hook, get the hook name and save the
            // indent so we can indent all injected lines properly.
            const std::string& hook_name = stripped_line.substr(1, stripped_line.size()-2);
            const std::string indent(line.size() - boost::trim_left_copy(line).size(), ' ');

            // Inject all registered template values into the hook
            if (injections.find(hook_name) != injections.end())
            {
                // Directly remove the value from the list so we can check which
                // hooks were not found in the template
                auto insertions = std::move(injections.at(hook_name));
                injections.erase(hook_name);

                if (insertions.size() > 0)
                {
                    // When we are in the main function, we have to make sure we
                    // use a seperate scope, so there are no conflicts with variable
                    // declarations
                    const std::string& header = indent + "/* Hook: " + hook_name + " */" + (in_main ? " {" : "");
                    parsed_lines.push_back(header);

                    for (const auto& line_to_insert: insertions)
                    {
                        if (line_to_insert.empty())
                        {
                            self.warn(std::string("Empty insertion a hook '") + hook_name + "'");
                            continue;
                        }

                        // Dont indent defines and pragmas
                        if (line_to_insert[0] == '#')
                            parsed_lines.push_back(line_to_insert);
                        else
                            parsed_lines.push_back(indent + line_to_insert);
                    }

                    if (in_main)
                        parsed_lines.push_back(indent + "}");
                }
            }
        }
        else
        {
            parsed_lines.push_back(boost::trim_right_copy(line));
        }
    }

    // Add a closing newline to the file
    // (C++: add newline when the file is written.)
    //parsed_lines.push_back("");

    // Warn the user about all unused hooks
    for (const auto& key_val: injections)
        self.warn(std::string("Hook '") + key_val.first + "' not found in template '" + template_src.to_os_generic() + "'!");

    // Write the constructed shader and load it back
    const std::string& temp_path = std::string("/$$rptemp/$$effect-") + cache_key + ".glsl";

    try
    {
        auto file = rppanda::open_write_file(temp_path, false, true);
        for (const auto& shader_content: parsed_lines)
            *file << shader_content << std::endl;
    }
    catch (const std::exception& err)
    {
        self.error(std::string("Error writing processed shader: ") + err.what());
    }

    return temp_path;
}

// ************************************************************************************************

std::shared_ptr<Effect> Effect::load(RenderPipeline& pipeline, const Filename& filename, const OptionType& options)
{
    const std::string& effect_hash = Impl::generate_hash(filename, options);

    auto found = Impl::global_cache_.find(effect_hash);
    if (found != Impl::global_cache_.end())
        return found->second;

    auto effect = std::make_shared<Effect>();
    effect->set_options(options);
    if (!effect->do_load(pipeline, filename))
    {
        RPObject::global_error("Effect", "Could not load effect!");
        return nullptr;
    }

    return effect;
}

const Effect::OptionType& Effect::get_default_options()
{
    return Impl::default_options_;
}

const std::vector<Effect::PassType>& Effect::get_passes()
{
    return Impl::passes_;
}

void Effect::add_pass(const PassType& pass, bool flag)
{
    Impl::passes_.push_back(pass);
    if (!Impl::default_options_.insert({ pass_option_prefix + pass.id, flag }).second)
        RPObject::global_warn("Effect", fmt::format("The pass is already added: {}", pass.id));
}

Effect::Effect(): RPObject("Effect"), impl_(std::make_unique<Impl>())
{
    Impl::effect_id_ += 1;
    impl_->options_ = Impl::default_options_;
}

Effect::Effect(Effect&&) = default;

Effect::~Effect() = default;

Effect& Effect::operator=(Effect&&) = default;

int Effect::get_effect_id() const
{
    return impl_->this_effect_id_;
}

bool Effect::get_option(const std::string& name) const
{
    return impl_->options_.at(name);
}

void Effect::set_options(const OptionType& options)
{
    for (const auto& pair: options)
    {
        if (impl_->options_.find(pair.first) == impl_->options_.end())
        {
            error("Unknown option: " + pair.first);
            continue;
        }
        impl_->options_[pair.first] = pair.second;
    }
}

bool Effect::do_load(RenderPipeline& pipeline, const Filename& filename)
{
    impl_->filename_ = filename;
    impl_->effect_name_ = impl_->convert_filename_to_name(filename);
    impl_->effect_hash_ = impl_->generate_hash(filename, impl_->options_);

    // Load the YAML file
    YAML::Node parsed_yaml;
    if (!rplibs::load_yaml_file(filename, parsed_yaml))
        return false;
    impl_->parse_content(pipeline, *this, parsed_yaml);

    // Construct a shader object for each pass
    for (const auto& pass_id_multiview: Impl::passes_)
    {
        const std::string& vertex_src = impl_->generated_shader_paths_.at("vertex-" + pass_id_multiview.id);
        const std::string& fragment_src = impl_->generated_shader_paths_.at("fragment-" + pass_id_multiview.id);
        std::string geometry_src;

        auto geometry_src_iter = impl_->generated_shader_paths_.find("geometry-" + pass_id_multiview.id);
        if (geometry_src_iter != impl_->generated_shader_paths_.end())
            geometry_src = geometry_src_iter->second;

        impl_->shader_objs_.insert_or_assign(pass_id_multiview.id, RPLoader::load_shader({vertex_src, fragment_src, geometry_src}));
    }

    return true;
}

Shader* Effect::get_shader_obj(const std::string& pass_id) const
{
    auto found = impl_->shader_objs_.find(pass_id);
    if (found != impl_->shader_objs_.end())
    {
        return found->second;
    }
    else
    {
        warn(std::string("Pass '") + pass_id + "' not found!");
        return nullptr;
    }
}

}
