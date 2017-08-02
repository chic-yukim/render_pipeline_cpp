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
 * This is C++ porting codes of direct/src/gui/DirectCheckBox.py
 */

#include <render_pipeline/rppanda/gui/direct_check_box.hpp>

#include <pgButton.h>

#include <render_pipeline/rppanda/util/image_input.hpp>
#include "render_pipeline/rppanda/gui/direct_gui_globals.hpp"

namespace rppanda {

TypeHandle DirectCheckBox::type_handle_;

DirectCheckBox::Options::Options(void)
{
    num_states = 4;
    state = NORMAL;
    relief = RAISED;
    inverted_frames = { 1 };

    press_effect = true;

    command_func = DirectCheckBox::command_func;
}

DirectCheckBox::DirectCheckBox(NodePath parent, const std::shared_ptr<Options>& options): DirectCheckBox(new PGButton(""), parent, options, get_class_type())
{
}

DirectCheckBox::DirectCheckBox(PGItem* gui_item, NodePath parent, const std::shared_ptr<Options>& options, const TypeHandle& type_handle):
    DirectButton(gui_item, parent, define_options(options), type_handle)
{
    // Call option initialization functions
    if (is_exact_type(type_handle))
    {
        initialise_options(options);
        frame_initialise_func();
    }
}

void DirectCheckBox::command_func(const Event* ev, void* user_data)
{
    const auto& options = std::dynamic_pointer_cast<Options>(reinterpret_cast<DirectCheckBox*>(user_data)->_options);

    options->is_checked = !options->is_checked;

    if (options->is_checked)
        reinterpret_cast<DirectCheckBox*>(user_data)->set_image(options->checked_image);
    else
        reinterpret_cast<DirectCheckBox*>(user_data)->set_image(options->unchecked_image);

    if (options->command)
    {
        // Pass any extra args to command
#if !defined(_MSC_VER) || _MSC_VER >= 1900
        options->command(std::shared_ptr<void>(user_data, [](auto){}));
#else
        options->command(std::shared_ptr<void>(user_data, [](void*) {}));
#endif
    }
}

const std::shared_ptr<DirectCheckBox::Options>& DirectCheckBox::define_options(const std::shared_ptr<Options>& options)
{
    return options;
}

void DirectCheckBox::initialise_options(const std::shared_ptr<Options>& options)
{
    DirectButton::initialise_options(options);

    f_init_ = true;
    f_init_ = false;
}

}
