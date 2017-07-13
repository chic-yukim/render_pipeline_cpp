/**
 * This is C++ porting codes of direct/src/gui/DirectButton.py
 */

#pragma once

#include <render_pipeline/rppanda/gui/direct_frame.hpp>

class PGButton;

namespace rppanda {

/**
 * DirectButton(parent) - Create a DirectGuiWidget which responds
 * to mouse clicks and execute a callback function if defined.
 */
class DirectButton: public DirectFrame
{
public:
    /** Parameter Container. */
    struct Options: public DirectFrame::Options
    {
        Options(void);

        /// Command to be called on button click.
        std::function<void(void*)> command;
        void* extra_args = nullptr;

        /// Which mouse buttons can be used to click the button.
        std::vector<int> command_buttons;

        /**
         * Can only be specified at time of widget contruction
         * Do the text/graphics appear to move when the button is clicked
         */
        bool press_effect = true;

    protected:
        friend class DirectButton;

        void (*command_func)(const Event*, void*) = nullptr;
    };

public:
    DirectButton(NodePath parent={}, const std::shared_ptr<Options>& options=std::make_shared<Options>());

    PGButton* get_gui_item(void) const;

    void set_command_buttons(void);

    static void command_func(const Event* ev, void* user_data);

    void set_click_sound(void);

    void set_rollover_sound(void);

protected:
    DirectButton(PGItem* gui_item, NodePath parent, const std::shared_ptr<Options>& options, const std::type_info& type_handle);

    void initialise_options(const std::shared_ptr<Options>& options);

private:
    const std::shared_ptr<Options>& define_options(const std::shared_ptr<Options>& options);

public:
    static const std::type_info& get_class_type(void) { return type_handle_; }
    virtual const std::type_info& get_type(void) const { return get_class_type(); }

private:
    static const std::type_info& type_handle_;
};

}