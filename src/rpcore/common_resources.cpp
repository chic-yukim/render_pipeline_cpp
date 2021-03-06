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

#include "rpcore/common_resources.hpp"

#include <unordered_map>

#include <lens.h>
#include <clockObject.h>
#include <dynamicTextFont.h>

#include "render_pipeline/rppanda/stdpy/file.hpp"
#include "render_pipeline/rpcore/globals.hpp"
#include "render_pipeline/rpcore/loader.hpp"
#include "render_pipeline/rpcore/render_pipeline.hpp"
#include "render_pipeline/rpcore/stage_manager.hpp"
#include "render_pipeline/rpcore/light_manager.hpp"
#include "render_pipeline/rpcore/image.hpp"
#include "render_pipeline/rppanda/showbase/showbase.hpp"

namespace rpcore {

CommonResources::CommonResources(RenderPipeline& pipeline): RPObject("CommonResources"), pipeline_(pipeline)
{
    showbase_ = Globals::base;

    load_fonts();
    load_textures();
    setup_inputs();
}

void CommonResources::write_config()
{
    const std::string& content = input_ubo_->generate_shader_code();
    try
    {
        // Try to write the temporary file
        (*rppanda::open_write_file("/$$rptemp/$$main_scene_data.inc.glsl", false, true)) << content;
    }
    catch (const std::exception& err)
    {
        error(std::string("Failed to write common resources shader configuration! ") + err.what());
    }
}

void CommonResources::load_skydome()
{
    Texture* skydome = RPLoader::load_texture("/$$rp/data/builtin_models/skybox/skybox.txo");
    skydome->set_wrap_u(SamplerState::WM_clamp);
    skydome->set_wrap_v(SamplerState::WM_clamp);
    pipeline_.get_stage_mgr()->add_input(ShaderInput("DefaultSkydome", skydome));
}

NodePath CommonResources::load_default_skybox()
{
    return RPLoader::load_model("/$$rp/data/builtin_models/skybox/skybox.bam");
}

void CommonResources::update()
{
    const bool stereo_mode = pipeline_.is_stereo_mode();
    const Lens* cam_lens = showbase_->get_cam_lens();
    static const LVecBase2i points[4] ={ LVecBase2i(-1, -1), LVecBase2i(1, -1), LVecBase2i(-1, 1), LVecBase2i(1, 1) };

    if (stereo_mode)
    {
        NodePath eye_path[2] ={
            showbase_->get_cam().find("left_eye"),
            showbase_->get_cam().find("right_eye")
        };

        // Matrix for stereo view space to stereo NDC.
        LMatrix4 stereo_proj_mat[2] ={
            cam_lens->get_projection_mat(Lens::StereoChannel::SC_left),
            cam_lens->get_projection_mat(Lens::StereoChannel::SC_right),
        };

        // Panda3D stereoscopic rendering
        if (cam_lens->get_interocular_distance() != 0)
        {
            // see: PerspectiveLens::do_compute_projection_mat
            const LVector3& iod = cam_lens->get_interocular_distance() * 0.5f * LVector3::left(cam_lens->get_coordinate_system());

            eye_path[0].set_mat(Globals::render, LMatrix4::translate_mat(iod) * showbase_->get_cam().get_mat(Globals::render));
            eye_path[1].set_mat(Globals::render, LMatrix4::translate_mat(-iod) * showbase_->get_cam().get_mat(Globals::render));

            // Panda3D stereo projection matrix is already applied for IOD. We need to fix this.
            // see: PerspectiveLens::do_compute_projection_mat
            stereo_proj_mat[0] = cam_lens->get_lens_mat_inv() * LMatrix4::translate_mat(iod) * cam_lens->get_lens_mat() * stereo_proj_mat[0];        // for left camera
            stereo_proj_mat[1] = cam_lens->get_lens_mat_inv() * LMatrix4::translate_mat(-iod) * cam_lens->get_lens_mat() * stereo_proj_mat[1];        // for right camera
        }

        // Get the current transform matrix of the camera
        const LMatrix4 view_mat[2] = {
            Globals::render.get_transform(eye_path[0])->get_mat(),
            Globals::render.get_transform(eye_path[1])->get_mat(),
        };

        // Compute the view matrix, but with a z-up coordinate system
        const LMatrix4& zup_conversion = LMatrix4::z_to_y_up_mat();
        input_ubo_->update_input("stereo_view_mat_z_up", view_mat[0] * zup_conversion, 0);
        input_ubo_->update_input("stereo_view_mat_z_up", view_mat[1] * zup_conversion, 1);

        // Compute the view matrix without the camera rotation
        LMatrix4 view_mat_billboard[2] = { view_mat[0], view_mat[1] };

        view_mat_billboard[0].set_row(0, LVecBase3(1, 0, 0));
        view_mat_billboard[0].set_row(1, LVecBase3(0, 1, 0));
        view_mat_billboard[0].set_row(2, LVecBase3(0, 0, 1));

        view_mat_billboard[1].set_row(0, LVecBase3(1, 0, 0));
        view_mat_billboard[1].set_row(1, LVecBase3(0, 1, 0));
        view_mat_billboard[1].set_row(2, LVecBase3(0, 0, 1));

        input_ubo_->update_input("stereo_view_mat_billboard", view_mat_billboard[0], 0);
        input_ubo_->update_input("stereo_view_mat_billboard", view_mat_billboard[1], 1);

        input_ubo_->update_input("stereo_camera_pos", eye_path[0].get_pos(Globals::render), 0);
        input_ubo_->update_input("stereo_camera_pos", eye_path[1].get_pos(Globals::render), 1);

        // Compute last view projection mat
        {
            const LMatrix4f& curr_vp_value = boost::get<const PTA_LMatrix4f&>(input_ubo_->get_input("stereo_view_proj_mat_no_jitter"))[0];
            input_ubo_->update_input("stereo_last_view_proj_mat_no_jitter", curr_vp_value, 0);
            LMatrix4f curr_vp = curr_vp_value;
            curr_vp.invert_in_place();
            const LMatrix4f curr_inv_vp = curr_vp;
            input_ubo_->update_input("stereo_last_inv_view_proj_mat_no_jitter", curr_inv_vp, 0);
        }
        {
            const LMatrix4f& curr_vp_value = boost::get<const PTA_LMatrix4f&>(input_ubo_->get_input("stereo_view_proj_mat_no_jitter"))[1];
            input_ubo_->update_input("stereo_last_view_proj_mat_no_jitter", curr_vp_value, 1);
            LMatrix4f curr_vp = curr_vp_value;
            curr_vp.invert_in_place();
            const LMatrix4f curr_inv_vp = curr_vp;
            input_ubo_->update_input("stereo_last_inv_view_proj_mat_no_jitter", curr_inv_vp, 1);
        }

        const LMatrix4 stereo_proj_mat_z_up[2] ={
            LMatrix4::y_to_z_up_mat() * stereo_proj_mat[0],
            LMatrix4::y_to_z_up_mat() * stereo_proj_mat[1],
        };

        const LMatrix4 stereo_view_proj_mat[2] = {
            view_mat[0] * stereo_proj_mat[0],
            view_mat[1] * stereo_proj_mat[1]
        };

        input_ubo_->update_input("stereo_ViewMatrix", view_mat[0], 0);
        input_ubo_->update_input("stereo_ViewMatrix", view_mat[1], 1);
        input_ubo_->update_input("stereo_ProjectionMatrix", stereo_proj_mat[0], 0);
        input_ubo_->update_input("stereo_ProjectionMatrix", stereo_proj_mat[1], 1);
        input_ubo_->update_input("stereo_ViewProjectionMatrix", stereo_view_proj_mat[0], 0);
        input_ubo_->update_input("stereo_ViewProjectionMatrix", stereo_view_proj_mat[1], 1);

        input_ubo_->update_input("stereo_ViewProjectionMatrixInverse", invert(stereo_view_proj_mat[0]), 0);
        input_ubo_->update_input("stereo_ViewProjectionMatrixInverse", invert(stereo_view_proj_mat[1]), 1);

        // Set the projection matrix as an input, but convert it to the correct
        // coordinate system before.
        input_ubo_->update_input("stereo_proj_mat", stereo_proj_mat_z_up[0], 0);
        input_ubo_->update_input("stereo_proj_mat", stereo_proj_mat_z_up[1], 1);

        // Set the inverse projection matrix
        input_ubo_->update_input("stereo_inv_proj_mat", invert(stereo_proj_mat_z_up[0]), 0);
        input_ubo_->update_input("stereo_inv_proj_mat", invert(stereo_proj_mat_z_up[1]), 1);

        LMatrix4 stereo_view_mat_inv[2] = { view_mat[0], view_mat[1] };
        stereo_view_mat_inv[0].invert_in_place();
        stereo_view_mat_inv[1].invert_in_place();

        // Remove jitter and set the new view projection mat
        stereo_proj_mat[0].set_cell(1, 0, 0.0);
        stereo_proj_mat[0].set_cell(1, 1, 0.0);
        input_ubo_->update_input("stereo_view_proj_mat_no_jitter", view_mat[0] * stereo_proj_mat[0], 0);

        stereo_proj_mat[1].set_cell(1, 0, 0.0);
        stereo_proj_mat[1].set_cell(1, 1, 0.0);
        input_ubo_->update_input("stereo_view_proj_mat_no_jitter", view_mat[1] * stereo_proj_mat[1], 1);

        // Compute frustum corners in the order BL, BR, TL, TR
        // XXX: DO NOT compute frustum calculation in stereo.
        // Stereoscopic projection matrix is NOT symmetric.
    }
    else
    {
        // Get the current transform matrix of the camera
        const LMatrix4& view_mat = Globals::render.get_transform(showbase_->get_cam())->get_mat();

        // Compute the view matrix, but with a z-up coordinate system
        const LMatrix4& zup_conversion = LMatrix4::z_to_y_up_mat();
        input_ubo_->update_input("view_mat_z_up", view_mat * zup_conversion);

        // Compute the view matrix without the camera rotation
        LMatrix4 view_mat_billboard(view_mat);
        view_mat_billboard.set_row(0, LVecBase3(1, 0, 0));
        view_mat_billboard.set_row(1, LVecBase3(0, 1, 0));
        view_mat_billboard.set_row(2, LVecBase3(0, 0, 1));
        input_ubo_->update_input("view_mat_billboard", view_mat_billboard);

        input_ubo_->update_input("camera_pos", showbase_->get_cam().get_pos(Globals::render));

        // Compute last view projection mat
        const LMatrix4& curr_vp_value = boost::get<const PTA_LMatrix4f&>(input_ubo_->get_input("view_proj_mat_no_jitter"))[0];
        input_ubo_->update_input("last_view_proj_mat_no_jitter", curr_vp_value);
        LMatrix4 curr_vp = curr_vp_value;
        curr_vp.invert_in_place();
        const LMatrix4 curr_inv_vp = curr_vp;
        input_ubo_->update_input("last_inv_view_proj_mat_no_jitter", curr_inv_vp);

        LMatrix4 proj_mat = showbase_->get_cam_lens()->get_projection_mat();

        // Set the projection matrix as an input, but convert it to the correct
        // coordinate system before.
        const LMatrix4& proj_mat_zup = LMatrix4::y_to_z_up_mat() * proj_mat;
        input_ubo_->update_input("proj_mat", proj_mat_zup);

        // Set the inverse projection matrix
        input_ubo_->update_input("inv_proj_mat", invert(proj_mat_zup));

        // Remove jitter and set the new view projection mat
        proj_mat.set_cell(1, 0, 0.0);
        proj_mat.set_cell(1, 1, 0.0);
        input_ubo_->update_input("view_proj_mat_no_jitter", view_mat * proj_mat);

        // Compute frustum corners in the order BL, BR, TL, TR
        LMatrix4 ws_frustum_directions;
        LMatrix4 vs_frustum_directions;
        const LMatrix4& inv_proj_mat = Globals::base->get_cam_lens()->get_projection_mat_inv();
        LMatrix4 view_mat_inv(view_mat);
        view_mat_inv.invert_in_place();

        for (size_t i = 0; i < std::extent<decltype(points)>::value; ++i)
        {
            const LVecBase4& result = inv_proj_mat.xform(LVecBase4(points[i][0], points[i][1], 1.0, 1.0));
            const LVecBase3& vs_dir = (zup_conversion.xform(result)).get_xyz().normalized();
            vs_frustum_directions.set_row(i, LVecBase4(vs_dir, 1));
            const LVecBase4& ws_dir = view_mat_inv.xform(LVecBase4(result.get_xyz(), 0));
            ws_frustum_directions.set_row(i, ws_dir);
        }

        input_ubo_->update_input("vs_frustum_directions", vs_frustum_directions);
        input_ubo_->update_input("ws_frustum_directions", ws_frustum_directions);
    }

    // Store the frame delta
    input_ubo_->update_input("frame_delta", static_cast<float>(Globals::clock->get_dt()));
    input_ubo_->update_input("smooth_frame_delta", static_cast<float>(1.0 / (std::max)(1e-5, Globals::clock->get_average_frame_rate())));
    input_ubo_->update_input("frame_time", static_cast<float>(Globals::clock->get_frame_time()));

    // Store the current film offset, we use this to compute the pixel-perfect
    // velocity, which is otherwise not possible.Usually this is always 0
    // except when SMAA and reprojection is enabled
    input_ubo_->update_input("current_film_offset", showbase_->get_cam_lens()->get_film_offset());
    input_ubo_->update_input("frame_index", Globals::clock->get_frame_count());

    input_ubo_->update_input("screen_size", Globals::resolution);
    input_ubo_->update_input("native_screen_size", Globals::native_resolution);
    input_ubo_->update_input("lc_tile_count", pipeline_.get_light_mgr()->get_num_tiles());
}

void CommonResources::load_fonts()
{
    Globals::font = RPLoader::load_font("/$$rp/data/font/Roboto-Medium.ttf");
    DynamicTextFont* dfont = DCAST(DynamicTextFont, Globals::font);
    dfont->set_pixels_per_unit(35);
    dfont->set_poly_margin(0.0f);
    dfont->set_texture_margin(1);
    dfont->set_bg(LColor(1, 1, 1, 0));
    dfont->set_fg(LColor(1, 1, 1, 1));
}

void CommonResources::load_textures()
{
    load_environment_cubemap();
    load_prefilter_brdf();
    load_skydome();
}

void CommonResources::load_environment_cubemap()
{
    Texture* envmap = RPLoader::load_cube_map("/$$rp/data/default_cubemap/cubemap.txo", true);
    envmap->set_minfilter(SamplerState::FT_linear_mipmap_linear);
    envmap->set_format(Texture::F_rgba16);
    envmap->set_magfilter(SamplerState::FT_linear);
    envmap->set_wrap_u(SamplerState::WM_repeat);
    envmap->set_wrap_v(SamplerState::WM_repeat);
    envmap->set_wrap_w(SamplerState::WM_repeat);
    pipeline_.get_stage_mgr()->add_input(ShaderInput("DefaultEnvmap", envmap));
}

void CommonResources::load_prefilter_brdf()
{
    static const std::unordered_map<std::string, std::string> luts ={
        { "PrefilteredBRDF", "slices/env_brdf_#.png" },
        { "PrefilteredMetalBRDF", "slices_metal/env_brdf.png" },
        { "PrefilteredCoatBRDF", "slices_coat/env_brdf.png" },
    };

    for (const auto& name_src: luts)
    {
        const std::string& name = name_src.first;
        const std::string& src = name_src.second;

        PT(Texture) brdf_tex = nullptr;

        if (src.find("#") != std::string::npos)
            brdf_tex = RPLoader::load_3d_texture("/$$rp/data/environment_brdf/" + src);
        else
            brdf_tex = RPLoader::load_texture("/$$rp/data/environment_brdf/" + src);

        brdf_tex->set_minfilter(SamplerState::FT_linear);
        brdf_tex->set_magfilter(SamplerState::FT_linear);
        brdf_tex->set_wrap_u(SamplerState::WM_clamp);
        brdf_tex->set_wrap_v(SamplerState::WM_clamp);
        brdf_tex->set_wrap_w(SamplerState::WM_clamp);
        brdf_tex->set_anisotropic_degree(0);
        pipeline_.get_stage_mgr()->add_input(ShaderInput(name, brdf_tex));
    }
}

void CommonResources::setup_inputs()
{
    const bool stereo_mode = pipeline_.is_stereo_mode();

    input_ubo_ = std::make_shared<GroupedInputBlock>("MainSceneData");

    if (stereo_mode)
    {
        input_ubo_->register_pta("stereo_camera_pos[2]", "vec3");
        input_ubo_->register_pta("stereo_view_proj_mat_no_jitter[2]", "mat4");
        input_ubo_->register_pta("stereo_last_view_proj_mat_no_jitter[2]", "mat4");
        input_ubo_->register_pta("stereo_last_inv_view_proj_mat_no_jitter[2]", "mat4");
        input_ubo_->register_pta("stereo_view_mat_z_up[2]", "mat4");
    }
    else
    {
        input_ubo_->register_pta("camera_pos", "vec3");
        input_ubo_->register_pta("view_proj_mat_no_jitter", "mat4");
        input_ubo_->register_pta("last_view_proj_mat_no_jitter", "mat4");
        input_ubo_->register_pta("last_inv_view_proj_mat_no_jitter", "mat4");
        input_ubo_->register_pta("view_mat_z_up", "mat4");
    }

    if (stereo_mode)
    {
        input_ubo_->register_pta("stereo_proj_mat[2]", "mat4");
        input_ubo_->register_pta("stereo_inv_proj_mat[2]", "mat4");
        input_ubo_->register_pta("stereo_view_mat_billboard[2]", "mat4");

        input_ubo_->register_pta("stereo_ViewMatrix[2]", "mat4");
        input_ubo_->register_pta("stereo_ProjectionMatrix[2]", "mat4");
        input_ubo_->register_pta("stereo_ViewProjectionMatrix[2]", "mat4");
        input_ubo_->register_pta("stereo_ViewProjectionMatrixInverse[2]", "mat4");
    }
    else
    {
        input_ubo_->register_pta("proj_mat", "mat4");
        input_ubo_->register_pta("inv_proj_mat", "mat4");
        input_ubo_->register_pta("view_mat_billboard", "mat4");
    }

    input_ubo_->register_pta("frame_delta", "float");
    input_ubo_->register_pta("smooth_frame_delta", "float");
    input_ubo_->register_pta("frame_time", "float");
    input_ubo_->register_pta("current_film_offset", "vec2");
    input_ubo_->register_pta("frame_index", "int");
    input_ubo_->register_pta("screen_size", "ivec2");
    input_ubo_->register_pta("native_screen_size", "ivec2");
    input_ubo_->register_pta("lc_tile_count", "ivec2");

    if (!stereo_mode)
    {
        input_ubo_->register_pta("vs_frustum_directions", "mat4");    // view space frustum
        input_ubo_->register_pta("ws_frustum_directions", "mat4");    // world space frustum
    }

    pipeline_.get_stage_mgr()->add_input_blocks(input_ubo_);

    if (!stereo_mode)
    {
        // Main camera and main render have to be regular inputs, since they are
        // used in the shaders by that name.
        pipeline_.get_stage_mgr()->add_input(ShaderInput("mainCam", showbase_->get_cam()));
        pipeline_.get_stage_mgr()->add_input(ShaderInput("mainRender", showbase_->get_render()));
    }

    // Set the correct frame rate interval
    Globals::clock->set_average_frame_rate_interval(3.0);
}

}
