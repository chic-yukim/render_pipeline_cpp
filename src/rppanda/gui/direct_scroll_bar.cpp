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
 * This is C++ porting codes of direct/src/gui/DirectScrollBar.py
 */

#include "render_pipeline/rppanda/gui/direct_scroll_bar.hpp"

#include <pgSliderBar.h>

#include "render_pipeline/rppanda/gui/direct_gui_globals.hpp"

namespace rppanda {

TypeHandle DirectScrollBar::type_handle_;

DirectScrollBar::Options::Options()
{
    state = DGG_NORMAL;
    frame_color = LColor(0.6, 0.6, 0.6, 1.0);
    orientation = DGG_HORIZONTAL;

    thumb_options = std::make_shared<DirectButton::Options>();
    inc_button_options = std::make_shared<DirectButton::Options>();
    dec_button_options = std::make_shared<DirectButton::Options>();
}

DirectScrollBar::DirectScrollBar(NodePath parent, const std::shared_ptr<Options>& options): DirectScrollBar(new PGSliderBar(""), parent, options, get_class_type())
{
}

DirectScrollBar::DirectScrollBar(PGItem* gui_item, NodePath parent, const std::shared_ptr<Options>& options, const TypeHandle& type_handle):
    DirectFrame(gui_item, parent, define_options(options), type_handle)
{
    options->thumb_options->border_width = options->border_width;
    _thumb = new DirectButton(*this, options->thumb_options);
    create_component("thumb", boost::any(_thumb));

    options->inc_button_options->border_width = options->border_width;
    _inc_button = new DirectButton(*this, options->inc_button_options);
    create_component("incButton", boost::any(_thumb));

    options->dec_button_options->border_width = options->border_width;
    _dec_button = new DirectButton(*this, options->dec_button_options);
    create_component("decButton", boost::any(_thumb));

    if (!_dec_button->get_frame_size() && _dec_button->get_bounds() == LVecBase4(0))
    {
        const auto& f = options->frame_size.value();
        if (options->orientation == DGG_HORIZONTAL)
            _dec_button->set_frame_size(LVecBase4(f[0]*0.05, f[1]*0.05, f[2], f[3]));
        else
            _dec_button->set_frame_size(LVecBase4(f[0], f[1], f[2]*0.05, f[3]*0.05));
    }

    if (!_inc_button->get_frame_size() && _inc_button->get_bounds() == LVecBase4(0))
    {
        const auto& f = options->frame_size.value();
        if (options->orientation == DGG_HORIZONTAL)
            _inc_button->set_frame_size(LVecBase4(f[0]*0.05, f[1]*0.05, f[2], f[3]));
        else
            _inc_button->set_frame_size(LVecBase4(f[0], f[1], f[2]*0.05, f[3]*0.05));
    }

    PGSliderBar* item = get_gui_item();
    item->set_thumb_button(_thumb->get_gui_item());
    item->set_left_button(_dec_button->get_gui_item());
    item->set_right_button(_inc_button->get_gui_item());

    // Bind command function
    this->bind(DGG_ADJUST, [this](const Event*){ command_func(); });

    // Call option initialization functions
    if (is_exact_type(type_handle))
    {
        initialise_options(options);
        frame_initialise_func();
    }
}

DirectScrollBar::~DirectScrollBar() = default;

PGSliderBar* DirectScrollBar::get_gui_item() const
{
    return DCAST(PGSliderBar, _gui_item);
}

void DirectScrollBar::set_range(const LVecBase2& range)
{
    auto options = static_cast<Options*>(options_.get());

    options->range = range;

    PGSliderBar* item = get_gui_item();
    float v = options->value;
    item->set_range(range[0], range[1]);
    item->set_value(v);
}

void DirectScrollBar::set_value(PN_stdfloat value)
{
    get_gui_item()->set_value(static_cast<Options*>(options_.get())->value=value);
}

void DirectScrollBar::set_scroll_size(PN_stdfloat scroll_size)
{
    get_gui_item()->set_scroll_size(static_cast<Options*>(options_.get())->scroll_size=scroll_size);
}

void DirectScrollBar::set_page_size(PN_stdfloat page_size)
{
    get_gui_item()->set_page_size(static_cast<Options*>(options_.get())->page_size=page_size);
}

void DirectScrollBar::set_orientation(const std::string& orientation)
{
    static_cast<Options*>(options_.get())->orientation = orientation;
    if (orientation == DGG_HORIZONTAL)
        get_gui_item()->set_axis(LVector3(1, 0, 0));
    else if (orientation == DGG_VERTICAL)
        get_gui_item()->set_axis(LVector3(0, 0, -1));
    else if (orientation == DGG_VERTICAL_INVERTED)
        get_gui_item()->set_axis(LVector3(0, 0, 1));
    else
        throw std::runtime_error(std::string("Invalid value for orientation: ") + orientation);
}

void DirectScrollBar::set_manage_buttons(bool manage_buttons)
{
    get_gui_item()->set_manage_pieces(static_cast<Options*>(options_.get())->manage_buttons=manage_buttons);
}

void DirectScrollBar::set_resize_thumb(bool resize_thumb)
{
    get_gui_item()->set_resize_thumb(static_cast<Options*>(options_.get())->resize_thumb=resize_thumb);
}

void DirectScrollBar::command_func()
{
    // Store the updated value in self['value']
    static_cast<Options*>(options_.get())->value = get_gui_item()->get_value();

    // TODO: implements
}

const std::shared_ptr<DirectScrollBar::Options>& DirectScrollBar::define_options(const std::shared_ptr<Options>& options)
{
    if (!options->frame_size)
    {
        if ((options->orientation == DGG_VERTICAL || options->orientation == DGG_VERTICAL_INVERTED))
        {
            // These are the default options for a vertical layout.
            options->frame_size = LVecBase4(-0.04f, 0.04f, -0.5f, 0.5f);
        }
        else
        {
            // These are the default options for a horizontal layout.
            options->frame_size = LVecBase4(-0.5f, 0.5f, -0.04f, 0.04f);
        }
    }

    return options;
}

void DirectScrollBar::initialise_options(const std::shared_ptr<Options>& options)
{
    DirectFrame::initialise_options(options);

    f_init_ = true;
    set_range(options->range);
    set_value(options->value);
    set_scroll_size(options->scroll_size);
    set_page_size(options->page_size);
    set_orientation(options->orientation);
    set_manage_buttons(options->manage_buttons);
    set_resize_thumb(options->resize_thumb);
    f_init_ = false;
}

}
