#include <render_pipeline/rppanda/gui/direct_button.h>

#include <pgButton.h>
#include <mouseButton.h>

#include <render_pipeline/rppanda/gui/direct_gui_globals.h>

namespace rppanda {

const std::type_info& DirectButton::_type_handle(typeid(DirectButton));

DirectButton::Options::Options(void)
{
	num_states = 4;
	state = NORMAL;
	relief = RAISED;
	inverted_frames = { 1 };

	command_buttons = { LMB, };

	command_func = DirectButton::command_func;
}

DirectButton::DirectButton(NodePath parent, const std::shared_ptr<Options>& options): DirectButton(new PGButton(""), parent, options, get_class_type())
{
}

DirectButton::DirectButton(PGItem* gui_item, NodePath parent, const std::shared_ptr<Options>& options, const std::type_info& type_handle):
	DirectFrame(gui_item, parent, define_options(options), type_handle)
{
	NodePath press_effect_np;
	if (options->press_effect)
	{
		press_effect_np = _state_node_path[1].attach_new_node("pressEffect", 1);
		_state_node_path[1] = press_effect_np;
	}

	// Call option initialization functions
	if (get_class_type() == type_handle)
	{
		initialise_options(options);
		frame_initialise_func();
	}

	// Now apply the scale.
	if (!press_effect_np.is_empty())
	{
		const LVecBase4f& bounds = get_bounds();
		float center_x = (bounds[0] + bounds[1]) / 2.0f;
		float center_y = (bounds[2] + bounds[3]) / 2.0f;

		// Make a matrix that scales about the point
		const LMatrix4f& mat = LMatrix4f::translate_mat(-center_x, 0, -center_y) *
			LMatrix4f::scale_mat(0.98f) *
			LMatrix4f::translate_mat(center_x, 0, center_y);
		press_effect_np.set_mat(mat);
	}
}

PGButton* DirectButton::get_gui_item(void) const
{
	return DCAST(PGButton, _gui_item);
}

void DirectButton::set_command_buttons(void)
{
	// Attach command function to specified buttons
	const auto& command_buttons = std::dynamic_pointer_cast<Options>(_options)->command_buttons;
	const auto& command_func = std::dynamic_pointer_cast<Options>(_options)->command_func;

	// Left mouse button
	if (std::find(command_buttons.begin(), command_buttons.end(), LMB) != command_buttons.end())
	{
		get_gui_item()->add_click_button(MouseButton::one());
		bind(B1CLICK, command_func, this);
	}
	else
	{
		unbind(B1CLICK);
		get_gui_item()->remove_click_button(MouseButton::one());
	}

	// Middle mouse button
	if (std::find(command_buttons.begin(), command_buttons.end(), MMB) != command_buttons.end())
	{
		get_gui_item()->add_click_button(MouseButton::two());
		bind(B2CLICK, command_func, this);
	}
	else
	{
		unbind(B2CLICK);
		get_gui_item()->remove_click_button(MouseButton::two());
	}

	// Right mouse button
	if (std::find(command_buttons.begin(), command_buttons.end(), RMB) != command_buttons.end())
	{
		get_gui_item()->add_click_button(MouseButton::three());
		bind(B3CLICK, command_func, this);
	}
	else
	{
		unbind(B3CLICK);
		get_gui_item()->remove_click_button(MouseButton::three());
	}
}

void DirectButton::command_func(const Event* ev, void* user_data)
{
	const auto& options = std::dynamic_pointer_cast<Options>(reinterpret_cast<DirectButton*>(user_data)->_options);
	if (options->command)
		options->command(options->extra_args);
}

void DirectButton::set_click_sound(void)
{
	// TODO: implement
}

void DirectButton::set_rollover_sound(void)
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

	_f_init = true;
	set_command_buttons();
	_f_init = false;
}

}
