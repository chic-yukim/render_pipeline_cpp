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

#include <dtoolbase.h>

#include "render_pipeline/rpcore/pluginbase/day_manager.hpp"

#include <regex>

#include <fmt/format.h>

#include "render_pipeline/rpcore/render_pipeline.hpp"
#include "render_pipeline/rpcore/stage_manager.hpp"
#include "render_pipeline/rpcore/pluginbase/manager.hpp"
#include "render_pipeline/rpcore/pluginbase/day_setting_types.hpp"
#include "render_pipeline/rpcore/util/shader_input_blocks.hpp"
#include "render_pipeline/rplibs/py_to_cpp.hpp"
#include "render_pipeline/rppanda/stdpy/file.hpp"

namespace rpcore {

class DayTimeManager::Impl
{
public:
    Impl(RenderPipeline& pipeline);

public:
    RenderPipeline& pipeline_;

    std::shared_ptr<GroupedInputBlock> input_ubo_;
    std::unordered_map<std::string, std::shared_ptr<DayBaseType>> setting_handles_;
    float time_ = 0.5f;
};

DayTimeManager::Impl::Impl(RenderPipeline& pipeline): pipeline_(pipeline)
{
    input_ubo_ = std::make_shared<GroupedInputBlock>("TimeOfDay");
}

// ************************************************************************************************

DayTimeManager::DayTimeManager(RenderPipeline& pipeline): RPObject("DayTimeManager"), impl_(std::make_unique<Impl>(pipeline))
{
}

DayTimeManager::~DayTimeManager() = default;

float DayTimeManager::get_time() const
{
    return impl_->time_;
}

void DayTimeManager::set_time(float time)
{
    impl_->time_ = rplibs::py_fmod(time, 1.0f);
}

void DayTimeManager::set_time(const std::string& time)
{
    std::smatch match;
    if (std::regex_match(time, match, std::regex("^([0-9]{1,2}):([0-9]{1,2})$")))
    {
        impl_->time_ = (std::stoi(match[1]) * 60.0f + std::stoi(match[2])) / (24.0f * 60.0f);
    }
    else
    {
        warn(std::string("Invalid time format: ") + time);
    }
}

std::string DayTimeManager::get_formatted_time() const
{
    float total_minutes = impl_->time_ * 24 * 60;
    return fmt::format("{:02d}:{:02d}", (int(total_minutes) / 60), (int(total_minutes) % 60));
}

void DayTimeManager::load_settings()
{
    auto plugin_mgr = impl_->pipeline_.get_plugin_mgr();
    for (const auto& plugin_id: plugin_mgr->get_enabled_plugins())
    {
        for (const auto& setting_handle: plugin_mgr->get_day_settings(plugin_id)->get<0>())
        {
            const std::string setting_id = plugin_id + "." + setting_handle.key;
            impl_->input_ubo_->register_pta(setting_id, setting_handle.value->get_glsl_type());
            impl_->setting_handles_[setting_id] = setting_handle.value;
        }
    }
    impl_->pipeline_.get_stage_mgr()->add_input_blocks(impl_->input_ubo_);

    // Generate UBO shader code
    try
    {
        // Try to write the temporary file
        (*rppanda::open_write_file("/$$rptemp/$$daytime_config.inc.glsl", false, true)) <<
            (impl_->input_ubo_->generate_shader_code());
    }
    catch (const std::exception& err)
    {
        error(std::string("Failed to write DayTimeManager UBO shader code! ") + err.what());
    }
}

void DayTimeManager::update()
{
    for (const auto& id_handle: impl_->setting_handles_)
    {
        // XXX: Find a better interface for this. Without this fix, colors
        // are in the range 0 .. 255 in the shader.
        DayBaseType::ValueType value = id_handle.second->get_shader_input_value(impl_->time_);
        if (value.second == 1)
            impl_->input_ubo_->update_input(id_handle.first, value.first[0]);
        else
            impl_->input_ubo_->update_input(id_handle.first, value.first);
    }
}

}
