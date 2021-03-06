/**
 * Copyright (c) 2008, Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Carnegie Mellon University nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * (This is the Modified BSD License.  See also
 * http://www.opensource.org/licenses/bsd-license.php )
 */

/**
 * This is C++ porting codes of direct/src/gui/DirectButton.py
 */

#include "render_pipeline/rppanda/gui/direct_button.hpp"

#include <pgButton.h>
#include <mouseButton.h>

#include "render_pipeline/rppanda/gui/direct_gui_globals.hpp"

namespace rppanda {

TypeHandle DirectButton::type_handle_;

DirectButton::Options::Options()
{
    num_states = 4;
    state = DGG_NORMAL;
    relief = DGG_RAISED;
    inverted_frames = { 1 };

    command_buttons = { DGG_LMB, };
}

DirectButton::DirectButton(NodePath parent, const std::shared_ptr<Options>& options): DirectButton(new PGButton(""), parent, options, get_class_type())
{
}

DirectButton::DirectButton(PGItem* gui_item, NodePath parent, const std::shared_ptr<Options>& options, const TypeHandle& type_handle):
    DirectFrame(gui_item, parent, define_options(options), type_handle)
{
    NodePath press_effect_np;
    if (options->press_effect)
    {
        press_effect_np = _state_node_path[1].attach_new_node("pressEffect", 1);
        _state_node_path[1] = press_effect_np;
    }

    // Call option initialization functions
    if (is_exact_type(type_handle))
    {
        initialise_options(options);
        frame_initialise_func();
    }

    // Now apply the scale.
    if (!press_effect_np.is_empty())
    {
        const auto& bounds = get_bounds();
        const auto center_x = (bounds[0] + bounds[1]) / 2.0f;
        const auto center_y = (bounds[2] + bounds[3]) / 2.0f;

        // Make a matrix that scales about the point
        const auto& mat = LMatrix4::translate_mat(-center_x, 0, -center_y) *
            LMatrix4::scale_mat(0.98f) *
            LMatrix4::translate_mat(center_x, 0, center_y);
        press_effect_np.set_mat(mat);
    }
}

DirectButton::~DirectButton() = default;

PGButton* DirectButton::get_gui_item() const
{
    return DCAST(PGButton, _gui_item);
}

void DirectButton::set_command_buttons()
{
    // Attach command function to specified buttons
    const auto& command_buttons = static_cast<Options*>(options_.get())->command_buttons;

    // Left mouse button
    if (std::find(command_buttons.begin(), command_buttons.end(), DGG_LMB) != command_buttons.end())
    {
        get_gui_item()->add_click_button(MouseButton::one());
        bind(DGG_B1CLICK, [this](const Event* ev) { command_func(ev); });
    }
    else
    {
        unbind(DGG_B1CLICK);
        get_gui_item()->remove_click_button(MouseButton::one());
    }

    // Middle mouse button
    if (std::find(command_buttons.begin(), command_buttons.end(), DGG_MMB) != command_buttons.end())
    {
        get_gui_item()->add_click_button(MouseButton::two());
        bind(DGG_B2CLICK, [this](const Event* ev) { command_func(ev); });
    }
    else
    {
        unbind(DGG_B2CLICK);
        get_gui_item()->remove_click_button(MouseButton::two());
    }

    // Right mouse button
    if (std::find(command_buttons.begin(), command_buttons.end(), DGG_RMB) != command_buttons.end())
    {
        get_gui_item()->add_click_button(MouseButton::three());
        bind(DGG_B3CLICK, [this](const Event* ev) { command_func(ev); });
    }
    else
    {
        unbind(DGG_B3CLICK);
        get_gui_item()->remove_click_button(MouseButton::three());
    }
}

void DirectButton::command_func(const Event*)
{
    auto options = static_cast<Options*>(options_.get());
    if (options->command)
        options->command();
}

void DirectButton::set_click_sound()
{
    // TODO: implement
}

void DirectButton::set_rollover_sound()
{
    // TODO: implement
}

const std::shared_ptr<DirectButton::Options>& DirectButton::define_options(const std::shared_ptr<Options>& options)
{
    return options;
}

void DirectButton::initialise_options(const std::shared_ptr<Options>& options)
{
    DirectFrame::initialise_options(options);

    f_init_ = true;
    set_command_buttons();
    f_init_ = false;
}

}
