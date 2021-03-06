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

#include "rpcore/gpu_command_queue.hpp"

#include "render_pipeline/rpcore/render_target.hpp"
#include "render_pipeline/rpcore/loader.hpp"
#include "render_pipeline/rpcore/render_pipeline.hpp"
#include "render_pipeline/rpcore/stage_manager.hpp"
#include "render_pipeline/rpcore/image.hpp"
#include "render_pipeline/rpcore/native/gpu_command.h"
#include "render_pipeline/rpcore/native/gpu_command_list.h"

namespace rpcore {

GPUCommandQueue::GPUCommandQueue(RenderPipeline& pipeline): RPObject("GPUCommandQueue"), pipeline_(pipeline)
{
    command_list_ = std::make_unique<GPUCommandList>();
    pta_num_commands_ = PTA_int::empty_array(1);
    create_data_storage();
    create_command_target();
    register_defines();
}

GPUCommandQueue::~GPUCommandQueue() = default;

size_t GPUCommandQueue::get_num_queued_commands() const
{
    return command_list_->get_num_commands();
}

void GPUCommandQueue::process_queue()
{
    PTA_uchar pointer = data_texture_->get_texture()->modify_ram_image();
    size_t num_commands_exec = command_list_->write_commands_to(pointer, commands_per_frame_);
    pta_num_commands_[0] = num_commands_exec;
}

void GPUCommandQueue::reload_shaders()
{
    command_target_->set_shader(RPLoader::load_shader({
        "/$$rp/shader/default_post_process.vert.glsl",
        "/$$rp/shader/process_command_queue.frag.glsl"}
    ));
}

void GPUCommandQueue::register_input(CPT_InternalName key, Texture* val)
{
    command_target_->set_shader_input(ShaderInput(std::move(key), val));
}

void GPUCommandQueue::register_defines()
{
    static_assert(GPUCommand::CommandType::CMD_type_count == 5, "GPUCommand::CommandType count is not the same with defined value");

    auto& defines = pipeline_.get_stage_mgr()->get_defines();
    defines["CMD_invalid"] = std::to_string(GPUCommand::CommandType::CMD_invalid);
    defines["CMD_store_light"] = std::to_string(GPUCommand::CommandType::CMD_store_light);
    defines["CMD_remove_light"] = std::to_string(GPUCommand::CommandType::CMD_remove_light);
    defines["CMD_store_source"] = std::to_string(GPUCommand::CommandType::CMD_store_source);
    defines["CMD_remove_sources"] = std::to_string(GPUCommand::CommandType::CMD_remove_sources);
    defines["GPU_CMD_INT_AS_FLOAT"] = std::string(GPUCommand::get_uses_integer_packing() ? "1": "0");
}

void GPUCommandQueue::create_data_storage()
{
    int command_buffer_size = commands_per_frame_ * 32;
    debug(std::string("Allocating command buffer of size ") + std::to_string(command_buffer_size));
    data_texture_ = Image::create_buffer("CommandQueue", command_buffer_size, "R32");
}

void GPUCommandQueue::create_command_target()
{
    command_target_ = std::make_unique<RenderTarget>("ExecCommandTarget");
    command_target_->set_size(1);
    command_target_->prepare_buffer();
    command_target_->set_shader_input(ShaderInput("CommandQueue", data_texture_->get_texture()));
    command_target_->set_shader_input(ShaderInput("commandCount", pta_num_commands_));
}

}
