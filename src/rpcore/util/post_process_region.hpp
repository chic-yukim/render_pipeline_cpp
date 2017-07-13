#pragma once

#include <nodePath.h>
#include <graphicsOutput.h>

#include <unordered_map>
#include <functional>

class CallbackObject;

namespace rpcore {

class PostProcessRegion
{
public:
	static PostProcessRegion* make(GraphicsOutput* internal_buffer);
	static PostProcessRegion* make(GraphicsOutput* internal_buffer, const LVecBase4f& dimensions);

public:
	PostProcessRegion(GraphicsOutput* internal_buffer);
	PostProcessRegion(GraphicsOutput* internal_buffer, const LVecBase4f& dimensions);

	void set_shader_input(const ShaderInput& inp, bool override_input=false);

	/** DisplayRegion functions with PostProcessRegion::region. */
	///@{
	std::function<void(int)>	set_sort;
	std::function<void(void)>	disable_clears;
	std::function<void(bool)>	set_active;
	std::function<void(bool)>	set_clear_depth_active;
	std::function<void(PN_stdfloat)>	set_clear_depth;
	std::function<void(const NodePath&)>	set_camera;
	std::function<void(bool)>	set_clear_color_active;
	std::function<void(const LColor&)>	set_clear_color;
	std::function<void(CallbackObject*)> set_draw_callback;
	///@}

	/** NodePath function with PostProcessRegion::tri. */
	///@{
	std::function<void(int)>	set_instance_count;
	/** NodePath::set_shader(const Shader* sha, int priority=0) */
	std::function<void(const Shader*,int)> set_shader;
	std::function<void(const RenderAttrib*,int)> set_attrib;
	///@}

	DisplayRegion* get_region(void) { return region; }
	NodePath get_node(void) const { return node; }

private:
	void init_function_pointers(void);
	void make_fullscreen_tri(void);
	void make_fullscreen_cam(void);

	PT(GraphicsOutput) _buffer;
	PT(DisplayRegion) region;
	NodePath node;
	NodePath tri;
	NodePath camera;
};

inline void PostProcessRegion::set_shader_input(const ShaderInput& inp, bool override_input)
{
	if (override_input)
		node.set_shader_input(inp);
	else
		tri.set_shader_input(inp);
}

}