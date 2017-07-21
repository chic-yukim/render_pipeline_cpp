/**
 * This is C++ porting codes of direct/src/showbase/ShowBase.py
 */

#include "render_pipeline/rppanda/showbase/showbase.hpp"

#include <cIntervalManager.h>
#include <pandaSystem.h>
#include <pandaFramework.h>
#include <windowFramework.h>
#include <depthTestAttrib.h>
#include <depthWriteAttrib.h>
#include <pgTop.h>
#include <asyncTaskManager.h>
#include <pgMouseWatcherBackground.h>
#include <orthographicLens.h>
#include <audioManager.h>
#include <throw_event.h>

#include "render_pipeline/rppanda/showbase/sfx_player.hpp"
#include "render_pipeline/rppanda/showbase/loader.hpp"

#include "rppanda/config_rppanda.hpp"

namespace rppanda {

static ShowBase* global_showbase = nullptr;

class ShowBase::Impl
{
public:
    static AsyncTask::DoneStatus ival_loop(GenericAsyncTask *task, void *user_data);
    static AsyncTask::DoneStatus audio_loop(GenericAsyncTask *task, void *user_data);

    void initailize(ShowBase* self);

    void setup_mouse(ShowBase* self);
    void setup_render_2dp(ShowBase* self);
    void setup_render_2d(ShowBase* self);

    void add_sfx_manager(AudioManager* extra_sfx_manager);
    void create_base_audio_managers(void);
    void enable_music(bool enable);

    Filename get_screenshot_filename(const std::string& name_prefix, bool default_filename) const;
    void send_screenshot_event(const Filename& filename) const;

public:
    std::shared_ptr<PandaFramework> panda_framework_;
    WindowFramework* window_framework_ = nullptr;

    rppanda::Loader* loader_ = nullptr;
    GraphicsEngine* graphics_engine_ = nullptr;
    GraphicsWindow* win_ = nullptr;

    std::shared_ptr<SfxPlayer> sfx_player_;
    PT(AudioManager) sfx_manager_;
    PT(AudioManager) music_manager_;

    std::vector<PT(AudioManager)> sfx_manager_list_;
    std::vector<bool> sfx_manager_is_valid_list_;

    bool want_render_2dp_;
    float config_aspect_ratio_;
    std::string window_type_;
    bool require_window_;

    NodePath render_2dp_;
    NodePath aspect_2dp_;
    NodePath pixel_2dp_;

    NodePath camera2dp_;
    NodePath cam2dp_;

    float a2dp_top_;
    float a2dp_bottom_;
    float a2dp_left_;
    float a2dp_right_;

    NodePath a2dp_top_center_;
    NodePath a2dp_bottom_center_;
    NodePath a2dp_left_center_;
    NodePath a2dp_right_center_;

    NodePath a2dp_top_left_;
    NodePath a2dp_top_right_;
    NodePath a2dp_bottom_left_;
    NodePath a2dp_bottom_right_;

    NodePath a2d_background_;
    float a2d_top_;
    float a2d_bottom_;
    float a2d_left_;
    float a2d_right_;

    NodePath a2d_top_center_;
    NodePath a2d_top_center_ns_;
    NodePath a2d_bottom_center_;
    NodePath a2d_bottom_center_ns_;
    NodePath a2d_left_center_;
    NodePath a2d_left_center_ns_;
    NodePath a2d_right_center_;
    NodePath a2d_right_center_ns_;

    NodePath a2d_top_left_;
    NodePath a2d_top_left_ns_;
    NodePath a2d_top_right_;
    NodePath a2d_top_right_ns_;
    NodePath a2d_bottom_left_;
    NodePath a2d_bottom_left_ns_;
    NodePath a2d_bottom_right_;
    NodePath a2d_bottom_right_ns_;

    NodePath mouse_watcher_;
    MouseWatcher* mouse_watcher_node_;

    bool app_has_audio_focus_ = true;
    bool music_manager_is_valid_ = false;
    bool sfx_active_ = false;
    bool music_active_ = false;

    bool backface_culling_enabled_;
    bool wireframe_enabled_;
};

AsyncTask::DoneStatus ShowBase::Impl::ival_loop(GenericAsyncTask *task, void *user_data)
{
    // Execute all intervals in the global ivalMgr.
    CIntervalManager::get_global_ptr()->step();
    return AsyncTask::DS_cont;
}

AsyncTask::DoneStatus ShowBase::Impl::audio_loop(GenericAsyncTask *task, void *user_data)
{
    Impl* impl = reinterpret_cast<Impl*>(user_data);

    if (impl->music_manager_)
    {
        impl->music_manager_->update();
    }

    for (auto& x: impl->sfx_manager_list_)
        x->update();

    return AsyncTask::DS_cont;
}

void ShowBase::Impl::initailize(ShowBase* self)
{
    if (global_showbase)
    {
        rppanda_cat.error() << "ShowBase was already created!" << std::endl;
        return;
    }

    rppanda_cat.debug() << "Creating ShowBase ..." << std::endl;

    if (panda_framework_->get_num_windows() == 0)
        window_framework_ = panda_framework_->open_window();
    else
        window_framework_ = panda_framework_->get_window(0);

    sfx_active_ = ConfigVariableBool("audio-sfx-active", true).get_value();
    music_active_ = ConfigVariableBool("audio-music-active", true).get_value();
    want_render_2dp_ = ConfigVariableBool("want-render2dp", true).get_value();

    // screenshot_extension in config_display.h

    // If the aspect ratio is 0 or None, it means to infer the
    // aspect ratio from the window size.
    // If you need to know the actual aspect ratio call base.getAspectRatio()

    // aspect_ratio in config_framework.h, but NOT exported.
    config_aspect_ratio_ = ConfigVariableDouble("aspect-ratio", 0).get_value();

    // This is set to the value of the window-type config variable, but may
    // optionally be overridden in the Showbase constructor.  Should either be
    // 'onscreen' (the default), 'offscreen' or 'none'.

    // window_type in config_framework.h, but NOT exported.
    window_type_ = ConfigVariableString("window-type", "onscreen").get_value();
    require_window_ = ConfigVariableBool("require-window", true).get_value();

    // The global graphics engine, ie. GraphicsEngine.getGlobalPtr()
    graphics_engine_ = GraphicsEngine::get_global_ptr();
    self->setup_render();
    self->setup_render_2d();
    self->setup_data_graph();

    if (want_render_2dp_)
        self->setup_render_2dp();

    win_ = window_framework_->get_graphics_window();

    // Open the default rendering window.
    self->open_default_window();

    // The default is trackball mode, which is more convenient for
    // ad-hoc development in Python using ShowBase.Applications
    // can explicitly call base.useDrive() if they prefer a drive
    // interface.
    self->use_trackball();

    loader_ = new rppanda::Loader(*self);

    app_has_audio_focus_ = true;

    self->create_base_audio_managers();

    global_showbase = self;

    self->restart();
}

void ShowBase::Impl::setup_mouse(ShowBase* self)
{
    self->setup_mouse_cb();

    mouse_watcher_ = window_framework_->get_mouse();
    mouse_watcher_node_ = DCAST(MouseWatcher, mouse_watcher_.node());

    // In C++, aspect2d has already mouse watcher.
    DCAST(PGTop, aspect_2dp_.node())->set_mouse_watcher(mouse_watcher_node_);
    DCAST(PGTop, self->get_pixel_2d().node())->set_mouse_watcher(mouse_watcher_node_);
    DCAST(PGTop, pixel_2dp_.node())->set_mouse_watcher(mouse_watcher_node_);
}

void ShowBase::Impl::setup_render_2dp(ShowBase* self)
{
    rppanda_cat.debug() << "Setup 2D nodes." << std::endl;

    render_2dp_ = NodePath("render2dp");

    // Set up some overrides to turn off certain properties which
    // we probably won't need for 2-d objects.

    // It's probably important to turn off the depth test, since
    // many 2-d objects will be drawn over each other without
    // regard to depth position.

    const RenderAttrib* dt = DepthTestAttrib::make(DepthTestAttrib::M_none);
    const RenderAttrib* dw = DepthWriteAttrib::make(DepthWriteAttrib::M_off);
    render_2dp_.set_depth_test(0);
    render_2dp_.set_depth_write(0);

    render_2dp_.set_material_off(1);
    render_2dp_.set_two_sided(1);

    // The normal 2-d DisplayRegion has an aspect ratio that
    // matches the window, but its coordinate system is square.
    // This means anything we parent to render2dp gets stretched.
    // For things where that makes a difference, we set up
    // aspect2dp, which scales things back to the right aspect
    // ratio along the X axis (Z is still from -1 to 1)
    PT(PGTop) aspect_2dp_pg_top = new PGTop("aspect2dp");
    aspect_2dp_ = render_2dp_.attach_new_node(aspect_2dp_pg_top);
    aspect_2dp_pg_top->set_start_sort(16384);

    const float aspect_ratio = self->get_aspect_ratio();
    aspect_2dp_.set_scale(1.0f / aspect_ratio, 1.0f, 1.0f);

    // The Z position of the top border of the aspect2dp screen.
    a2dp_top_ = 1.0f;
    // The Z position of the bottom border of the aspect2dp screen.
    a2dp_bottom_ = -1.0f;
    // The X position of the left border of the aspect2dp screen.
    a2dp_left_ = -aspect_ratio;
    // The X position of the right border of the aspect2dp screen.
    a2dp_right_ = aspect_ratio;

    a2dp_top_center_ = aspect_2dp_.attach_new_node("a2dpTopCenter");
    a2dp_bottom_center_ = aspect_2dp_.attach_new_node("a2dpBottomCenter");
    a2dp_left_center_ = aspect_2dp_.attach_new_node("a2dpLeftCenter");
    a2dp_right_center_ = aspect_2dp_.attach_new_node("a2dpRightCenter");

    a2dp_top_left_ = aspect_2dp_.attach_new_node("a2dpTopLeft");
    a2dp_top_right_ = aspect_2dp_.attach_new_node("a2dpTopRight");
    a2dp_bottom_left_ = aspect_2dp_.attach_new_node("a2dpBottomLeft");
    a2dp_bottom_right_ = aspect_2dp_.attach_new_node("a2dpBottomRight");

    // Put the nodes in their places
    a2dp_top_center_.set_pos(0, 0, a2dp_top_);
    a2dp_bottom_center_.set_pos(0, 0, a2dp_bottom_);
    a2dp_left_center_.set_pos(a2dp_left_, 0, 0);
    a2dp_right_center_.set_pos(a2dp_right_, 0, 0);

    a2dp_top_left_.set_pos(a2dp_left_, 0, a2dp_top_);
    a2dp_top_right_.set_pos(a2dp_right_, 0, a2dp_top_);
    a2dp_bottom_left_.set_pos(a2dp_left_, 0, a2dp_bottom_);
    a2dp_bottom_right_.set_pos(a2dp_right_, 0, a2dp_bottom_);

    // This special root, pixel2d, uses units in pixels that are relative
    // to the window. The upperleft corner of the window is (0, 0),
    // the lowerleft corner is (xsize, -ysize), in this coordinate system.
    PT(PGTop) pixel_2dp_pg_top = new PGTop("pixel2dp");
    pixel_2dp_ = render_2dp_.attach_new_node(pixel_2dp_pg_top);
    pixel_2dp_pg_top->set_start_sort(16384);
    pixel_2dp_.set_pos(-1, 0, 1);
    const LVecBase2i& size = self->get_size();
    float xsize = size.get_x();
    float ysize = size.get_y();
    if (xsize > 0 && ysize > 0)
        pixel_2dp_.set_scale(2.0f / xsize, 1.0f, 2.0f / ysize);
}

void ShowBase::Impl::setup_render_2d(ShowBase* self)
{
    // Window framework already created render2d.
    NodePath render2d = self->get_render_2d();

    // Window framework already created aspect2d.
    NodePath aspect_2d = self->get_aspect_2d();

    a2d_background_ = aspect_2d.attach_new_node("a2d_background");

    const float aspect_ratio = self->get_aspect_ratio();

    // The Z position of the top border of the aspect2d screen.
    a2d_top_ = 1.0f;
    // The Z position of the bottom border of the aspect2d screen.
    a2d_bottom_ = -1.0f;
    // The X position of the left border of the aspect2d screen.
    a2d_left_ = -aspect_ratio;
    // The X position of the right border of the aspect2d screen.
    a2d_right_ = aspect_ratio;

    a2d_top_center_ = aspect_2d.attach_new_node("a2d_top_center");
    a2d_top_center_ns_ = aspect_2d.attach_new_node("a2d_top_center_ns");
    a2d_bottom_center_ = aspect_2d.attach_new_node("a2d_bottom_center");
    a2d_bottom_center_ns_ = aspect_2d.attach_new_node("a2d_bottom_center_ns");
    a2d_left_center_ = aspect_2d.attach_new_node("a2d_left_center");
    a2d_left_center_ns_ = aspect_2d.attach_new_node("a2d_left_center_ns");
    a2d_right_center_ = aspect_2d.attach_new_node("a2d_right_center");
    a2d_right_center_ns_ = aspect_2d.attach_new_node("a2d_right_center_ns");

    a2d_top_left_ = aspect_2d.attach_new_node("a2d_top_left");
    a2d_top_left_ns_ = aspect_2d.attach_new_node("a2d_top_left_ns");
    a2d_top_right_ = aspect_2d.attach_new_node("a2d_top_right");
    a2d_top_right_ns_ = aspect_2d.attach_new_node("a2d_top_right_ns");
    a2d_bottom_left_ = aspect_2d.attach_new_node("a2d_bottom_left");
    a2d_bottom_left_ns_ = aspect_2d.attach_new_node("a2d_bottom_left_ns");
    a2d_bottom_right_ = aspect_2d.attach_new_node("a2d_bottom_right");
    a2d_bottom_right_ns_ = aspect_2d.attach_new_node("a2d_bottom_right_ns");

    // Put the nodes in their places
    a2d_top_center_.set_pos(0, 0, a2d_top_);
    a2d_top_center_ns_.set_pos(0, 0, a2d_top_);
    a2d_bottom_center_.set_pos(0, 0, a2d_bottom_);
    a2d_bottom_center_ns_.set_pos(0, 0, a2d_bottom_);
    a2d_left_center_.set_pos(a2d_left_, 0, 0);
    a2d_left_center_ns_.set_pos(a2d_left_, 0, 0);
    a2d_right_center_.set_pos(a2d_right_, 0, 0);
    a2d_right_center_ns_.set_pos(a2d_right_, 0, 0);

    a2d_top_left_.set_pos(a2d_left_, 0, a2d_top_);
    a2d_top_left_ns_.set_pos(a2d_left_, 0, a2d_top_);
    a2d_top_right_.set_pos(a2d_right_, 0, a2d_top_);
    a2d_top_right_ns_.set_pos(a2d_right_, 0, a2d_top_);
    a2d_bottom_left_.set_pos(a2d_left_, 0, a2d_bottom_);
    a2d_bottom_left_ns_.set_pos(a2d_left_, 0, a2d_bottom_);
    a2d_bottom_right_.set_pos(a2d_right_, 0, a2d_bottom_);
    a2d_bottom_right_ns_.set_pos(a2d_right_, 0, a2d_bottom_);

    // pixel2d is created in window framework.
}

void ShowBase::Impl::add_sfx_manager(AudioManager* extra_sfx_manager)
{
    // keep a list of sfx manager objects to apply settings to,
    // since there may be others in addition to the one we create here
    sfx_manager_list_.push_back(extra_sfx_manager);
    bool new_sfx_manager_is_valid = extra_sfx_manager && extra_sfx_manager->is_valid();
    sfx_manager_is_valid_list_.push_back(new_sfx_manager_is_valid);
    if (new_sfx_manager_is_valid)
        extra_sfx_manager->set_active(sfx_active_);
}

void ShowBase::Impl::create_base_audio_managers(void)
{
    rppanda_cat.debug() << "Creating base audio manager ..." << std::endl;

    sfx_player_ = std::make_shared<SfxPlayer>();
    sfx_manager_ = AudioManager::create_AudioManager();
    add_sfx_manager(sfx_manager_);

    music_manager_ = AudioManager::create_AudioManager();
    music_manager_is_valid_ = music_manager_ && music_manager_->is_valid();
    if (music_manager_is_valid_)
    {
        // ensure only 1 midi song is playing at a time:
        music_manager_->set_concurrent_sound_limit(1);
        music_manager_->set_active(music_active_);
    }
}

void ShowBase::Impl::enable_music(bool enable)
{
    // don't setActive(1) if no audiofocus
    if (app_has_audio_focus_ && music_manager_is_valid_)
        music_manager_->set_active(enable);
    music_active_ = enable;
    if (enable)
    {
        // This is useful when we want to play different music
        // from what the manager has queued
        throw_event_directly(*EventHandler::get_global_event_handler(), "MusicEnabled");
        rppanda_cat.debug() << "Enabling music" << std::endl;
    }
    else
    {
        rppanda_cat.debug() << "Disabling music" << std::endl;
    }
}

Filename ShowBase::Impl::get_screenshot_filename(const std::string& name_prefix, bool default_filename) const
{
    if (default_filename)
        return GraphicsOutput::make_screenshot_filename(name_prefix);
    else
        return Filename(name_prefix);
}

void ShowBase::Impl::send_screenshot_event(const Filename& filename) const
{
    // Announce to anybody that a screenshot has been taken
    throw_event_directly(*EventHandler::get_global_event_handler(), "screenshot", EventParameter(filename));
}

// ************************************************************************************************

ShowBase::ShowBase(int& argc, char**& argv): impl_(std::make_unique<Impl>())
{
    impl_->panda_framework_ = std::make_shared<PandaFramework>();
    impl_->panda_framework_->open_framework(argc, argv);
    impl_->initailize(this);
}

ShowBase::ShowBase(PandaFramework* framework): impl_(std::make_unique<Impl>())
{
#if _MSC_VER >= 1900
    impl_->panda_framework_ = std::shared_ptr<PandaFramework>(framework, [](auto){});
#else
    impl_->panda_framework_ = std::shared_ptr<PandaFramework>(framework, [](PandaFramework*){});
#endif
    impl_->initailize(this);
}

ShowBase::~ShowBase(void)
{
    shutdown();

    if (impl_->music_manager_)
    {
        impl_->music_manager_->shutdown();
        impl_->music_manager_.clear();
        for (auto& manager: impl_->sfx_manager_list_)
            manager->shutdown();
        impl_->sfx_manager_list_.clear();
        impl_->sfx_manager_is_valid_list_.clear();
    }

    if (impl_->loader_)
    {
        delete impl_->loader_;
        impl_->loader_ = nullptr;
    }

    // will remove in PandaFramework::~PandaFramework
    //impl_->graphics_engine_->remove_all_windows();

    global_showbase = nullptr;
}

void ShowBase::setup_render_2d(void) { impl_->setup_render_2d(this); }
void ShowBase::setup_render_2dp(void) { impl_->setup_render_2dp(this); }
void ShowBase::setup_mouse(void) { impl_->setup_mouse(this); }
void ShowBase::create_base_audio_managers(void) { impl_->create_base_audio_managers(); }
void ShowBase::add_sfx_manager(AudioManager* extra_sfx_manager) { impl_->add_sfx_manager(extra_sfx_manager); }
void ShowBase::enable_music(bool enable) { impl_->enable_music(enable); }

ShowBase* ShowBase::get_global_ptr(void)
{
    return global_showbase;
}

PandaFramework* ShowBase::get_panda_framework(void) const
{
    return impl_->panda_framework_.get();
}

WindowFramework* ShowBase::get_window_framework(void) const
{
    return impl_->window_framework_;
}

rppanda::Loader* ShowBase::get_loader(void) const
{
    return impl_->loader_;
}

GraphicsEngine* ShowBase::get_graphics_engine(void) const
{
    return impl_->graphics_engine_;
}

GraphicsWindow* ShowBase::get_win(void) const
{
    return impl_->win_;
}

const std::vector<PT(AudioManager)>& ShowBase::get_sfx_manager_list(void) const
{
    return impl_->sfx_manager_list_;
}

SfxPlayer* ShowBase::get_sfx_player(void) const
{
    return impl_->sfx_player_.get();
}

AudioManager* ShowBase::get_music_manager(void) const
{
    return impl_->music_manager_;
}

NodePath ShowBase::get_render(void) const
{
    return impl_->window_framework_->get_render();
}

NodePath ShowBase::get_render_2d(void) const
{
    return impl_->window_framework_->get_render_2d();
}

NodePath ShowBase::get_aspect_2d(void) const
{
    return impl_->window_framework_->get_aspect_2d();
}

NodePath ShowBase::get_pixel_2d(void) const
{
    return impl_->window_framework_->get_pixel_2d();
}

NodePath ShowBase::get_render_2dp(void) const
{
    return impl_->render_2dp_;
}

NodePath ShowBase::get_pixel_2dp(void) const
{
    return impl_->pixel_2dp_;
}

float ShowBase::get_config_aspect_ratio(void) const
{
    return impl_->config_aspect_ratio_;
}

AsyncTaskManager* ShowBase::get_task_mgr(void) const
{
    return AsyncTaskManager::get_global_ptr();
}

bool ShowBase::open_default_window(void)
{
    open_main_window();

    return impl_->win_ != nullptr;
}

void ShowBase::open_main_window(void)
{
    if (impl_->win_)
    {
        setup_mouse();
        make_camera2dp(impl_->win_);
    }
}

void ShowBase::setup_render(void)
{
    // C++ sets already render node.
    //self.render.setAttrib(RescaleNormalAttrib.makeDefault())
    //self.render.setTwoSided(0)

    NodePath render = get_render();
    impl_->backface_culling_enabled_ = render.get_two_sided();
    //textureEnabled = 1;
    impl_->wireframe_enabled_ = render.get_render_mode() == RenderModeAttrib::M_wireframe;
}

NodePath ShowBase::make_camera2dp(GraphicsWindow* win, int sort, const LVecBase4f& display_region,
    const LVecBase4f& coords, Lens* lens, const std::string& camera_name)
{
    rppanda_cat.debug() << "Making 2D camera ..." << std::endl;

    DisplayRegion* dr = win->make_mono_display_region(display_region);
    dr->set_sort(sort);

    // Unlike render2d, we don't clear the depth buffer for
    // render2dp.Caveat emptor.

    dr->set_incomplete_render(false);

    // Now make a new Camera node.
    PT(Camera) cam2d_node;
    if (camera_name.empty())
        cam2d_node = new Camera("cam2dp");
    else
        cam2d_node = new Camera(std::string("cam2dp_") + camera_name);

    const float left = coords.get_x();
    const float right = coords.get_y();
    const float bottom = coords.get_z();
    const float top = coords.get_w();

    if (!lens)
    {
        lens = new OrthographicLens;
        lens->set_film_size(right - left, top - bottom);
        lens->set_film_offset((right + left) * 0.5, (top + bottom) * 0.5);
        lens->set_near_far(-1000, 1000);
    }
    cam2d_node->set_lens(lens);

    // self.camera2d is the analog of self.camera, although it's
    // not as clear how useful it is.
    if (impl_->camera2dp_.is_empty())
        impl_->camera2dp_ = impl_->render_2dp_.attach_new_node("camera2dp");

    NodePath camera2dp = impl_->camera2dp_.attach_new_node(cam2d_node);
    dr->set_camera(camera2dp);

    if (impl_->cam2dp_.is_empty())
        impl_->cam2dp_ = camera2dp;

    return camera2dp;
}

void ShowBase::setup_data_graph(void)
{

}

void ShowBase::setup_mouse_cb(void)
{
    // Note that WindowFramework::get_mouse

    impl_->window_framework_->enable_keyboard();
}

void ShowBase::toggle_backface(void)
{
    if (impl_->backface_culling_enabled_)
        backface_culling_off();
    else
        backface_culling_on();
}

void ShowBase::backface_culling_on(void)
{
    if (!impl_->backface_culling_enabled_)
        get_render().set_two_sided(false);
    impl_->backface_culling_enabled_ = true;
}

void ShowBase::backface_culling_off(void)
{
    if (!impl_->backface_culling_enabled_)
        get_render().set_two_sided(true);
    impl_->backface_culling_enabled_ = false;
}

void ShowBase::toggle_wireframe(void)
{
    if (impl_->wireframe_enabled_)
        wireframe_off();
    else
        wireframe_on();
}

void ShowBase::wireframe_on(void)
{
    NodePath render = get_render();
    render.set_render_mode_wireframe(100);
    render.set_two_sided(1);
    impl_->wireframe_enabled_ = true;
}

void ShowBase::wireframe_off(void)
{
    NodePath render = get_render();
    render.clear_render_mode();
    render.set_two_sided(!impl_->backface_culling_enabled_);
    impl_->wireframe_enabled_ = false;
}

float ShowBase::get_aspect_ratio(GraphicsOutput* win) const
{
    // If the config it set, we return that
    if (impl_->config_aspect_ratio_)
        return impl_->config_aspect_ratio_;

    float aspect_ratio = 1.0f;

    if (!win)
        win = impl_->win_;

    if (win && win->has_size() && win->get_sbs_left_y_size() != 0)
    {
        aspect_ratio = float(win->get_sbs_left_x_size()) / win->get_sbs_left_y_size();
    }
    else
    {
        WindowProperties props;
        if (!win && !DCAST(GraphicsWindow, win))
        {
            props = WindowProperties::get_default();
        }
        else
        {
            props = DCAST(GraphicsWindow, win)->get_requested_properties();
            if (!props.has_size())
                props = WindowProperties::get_default();
        }

        if (props.has_size() && props.get_y_size() != 0)
            aspect_ratio = float(props.get_x_size()) / props.get_y_size();
    }

    return aspect_ratio == 0 ? 1 : aspect_ratio;
}

const LVecBase2i& ShowBase::get_size(GraphicsOutput* win) const
{
    if (!win)
        win = impl_->win_;

    if (win && win->has_size())
    {
        return win->get_size();
    }
    else
    {
        WindowProperties props;
        if (!win && !DCAST(GraphicsWindow, win))
        {
            props = WindowProperties::get_default();
        }
        else
        {
            props = DCAST(GraphicsWindow, win)->get_requested_properties();
            if (!props.has_size())
                props = WindowProperties::get_default();
        }

        if (props.has_size())
        {
            return props.get_size();
        }
        else
        {
            static LVecBase2i zero_size(0);
            return zero_size;
        }
    }
}

NodePath ShowBase::get_camera(void) const
{
    return impl_->window_framework_->get_camera_group();
}

NodePath ShowBase::get_cam(int index) const
{
    return NodePath(impl_->window_framework_->get_camera(index));
}

Camera* ShowBase::get_cam_node(int index) const
{
    return impl_->window_framework_->get_camera(index);
}

Lens* ShowBase::get_cam_lens(int cam_index, int lens_index) const
{
    return get_cam_node(cam_index)->get_lens(lens_index);
}

MouseWatcher* ShowBase::get_mouse_watcher_node(void) const
{
    return impl_->mouse_watcher_node_;
}

NodePath ShowBase::get_button_thrower(void) const
{
    // Node that WindowFramework::enable_keyboard
    return impl_->window_framework_->get_mouse().find("kb-events");
}

const NodePath& ShowBase::get_data_root(void) const
{
    return impl_->panda_framework_->get_data_root();
}

PandaNode* ShowBase::get_data_root_node(void) const
{
    return get_data_root().node();
}

void ShowBase::restart(void)
{
    rppanda_cat.debug() << "Re-staring ShowBase ..." << std::endl;

    shutdown();

    add_task(Impl::ival_loop, nullptr, "ival_loop", 20);

    // the audioLoop updates the positions of 3D sounds.
    // as such, it needs to run after the cull traversal in the igLoop.
    add_task(Impl::audio_loop, impl_.get(), "audio_loop", 60);
}

void ShowBase::shutdown(void)
{
    rppanda_cat.debug() << "Shutdown ShowBase ..." << std::endl;

    auto task_mgr = get_task_mgr();
    AsyncTask* task = nullptr;

    if (task = get_task_mgr()->find_task("audio_loop"))
        task_mgr->remove(task);
    if (task = get_task_mgr()->find_task("ival_loop"))
        task_mgr->remove(task);
}

void ShowBase::disable_mouse(void)
{
    // Note that WindowFramework::setup_trackball.
    NodePath tball2cam = impl_->window_framework_->get_mouse().find("trackball/tball2cam");
    if (!tball2cam.is_empty())
        tball2cam.detach_node();
}

void ShowBase::use_trackball(void)
{
    impl_->window_framework_->setup_trackball();
}

void ShowBase::set_all_sfx_enables(bool enable)
{
    for (size_t k=0, k_end=impl_->sfx_manager_list_.size(); k < k_end; ++k)
    {
        if (impl_->sfx_manager_is_valid_list_[k])
            impl_->sfx_manager_list_[k]->set_active(enable);
    }
}

void ShowBase::disable_all_audio(void)
{
    impl_->app_has_audio_focus_ = false;
    set_all_sfx_enables(false);
    if (impl_->music_manager_is_valid_)
        impl_->music_manager_->set_active(false);
    rppanda_cat.debug() << "Disabling audio" << std::endl;
}

void ShowBase::enable_all_audio(void)
{
    impl_->app_has_audio_focus_ = true;
    set_all_sfx_enables(impl_->sfx_active_);
    if (impl_->music_manager_is_valid_)
        impl_->music_manager_->set_active(impl_->music_active_);
    rppanda_cat.debug() << "Enabling audio" << std::endl;
}

void ShowBase::enable_sound_effects(bool enable_sound_effects)
{
    // don't setActive(1) if no audiofocus
    if (impl_->app_has_audio_focus_ || !enable_sound_effects)
        set_all_sfx_enables(enable_sound_effects);
    impl_->sfx_active_ = enable_sound_effects;
    if (enable_sound_effects)
        rppanda_cat.debug() << "Enabling sound effects" << std::endl;
    else
        rppanda_cat.debug() << "Disabling sound effects" << std::endl;
}

void ShowBase::play_sfx(AudioSound* sfx, bool looping, bool interrupt, boost::optional<float> volume,
    float time, boost::optional<NodePath> node, boost::optional<NodePath> listener_node,
    boost::optional<float> cutoff)
{
    impl_->sfx_player_->play_sfx(sfx, looping, interrupt, volume, time, node, listener_node, cutoff);
}

void ShowBase::play_music(AudioSound* music, bool looping, bool interrupt, float time, boost::optional<float> volume)
{
    if (!music)
        return;

    if (volume)
        music->set_volume(volume.get());

    if (interrupt || (music->status() != AudioSound::PLAYING))
    {
        music->set_time(time);
        music->set_loop(looping);
        music->play();
    }
}

Filename ShowBase::screenshot(GraphicsOutput* source, const std::string& name_prefix, bool default_filename,
    const std::string& image_comment)
{
    if (!source)
        source = impl_->win_;

    Filename filename = impl_->get_screenshot_filename(name_prefix, default_filename);

    bool saved = source->save_screenshot(filename, image_comment);
    if (saved)
    {
        impl_->send_screenshot_event(filename);
        return filename;
    }

    return "";
}

Filename ShowBase::screenshot(Texture* source, const std::string& name_prefix, bool default_filename,
    const std::string& image_comment)
{
    if (!source)
    {
        rppanda_cat.error() << "ShowBase::screenshot source is nullptr." << std::endl;
        return "";
    }

    Filename filename = impl_->get_screenshot_filename(name_prefix, default_filename);

    bool saved = false;
    if (source->get_z_size() > 1)
        saved = source->write(filename, 0, 0, true, false);
    else
        saved = source->write(filename);

    if (saved)
    {
        impl_->send_screenshot_event(filename);
        return filename;
    }

    return "";
}

Filename ShowBase::screenshot(DisplayRegion* source, const std::string& name_prefix, bool default_filename,
    const std::string& image_comment)
{
    if (!source)
    {
        rppanda_cat.error() << "ShowBase::screenshot source is nullptr." << std::endl;
        return "";
    }

    Filename filename = impl_->get_screenshot_filename(name_prefix, default_filename);

    bool saved = source->save_screenshot(filename, image_comment);
    if (saved)
    {
        impl_->send_screenshot_event(filename);
        return filename;
    }

    return "";
}

void ShowBase::run(void)
{
    impl_->panda_framework_->main_loop();
}

}
