/**
 * This is C++ porting codes of direct/src/interval/SoundInterval.py
 */

#include "render_pipeline/rppanda/interval/sound_interval.hpp"

#include "render_pipeline/rppanda/showbase/showbase.hpp"
#include "render_pipeline/rppanda/showbase/sfx_player.hpp"

namespace rppanda {

size_t SoundInterval::sound_num_ = 1;

TypeHandle SoundInterval::type_handle_;

SoundInterval::SoundInterval(AudioSound* sound, bool loop, double duration,
    const boost::optional<std::string>& name, double volume, double start_time,
    boost::optional<NodePath> node, bool seamless_loop, boost::optional<NodePath> listener_node,
    boost::optional<float> cut_off): CInterval("unnamed", 0.0f, false)
{
    // Generate unique name
    std::string id = "Sound-" + std::to_string(SoundInterval::sound_num_);
    SoundInterval::sound_num_ += 1;

    // Record instance variables
    sound_ = sound;

    if (sound_)
        sound_duration_ = sound_->length();
    else
        sound_duration_ = 0;

    f_loop_ = loop;
    volume_ = volume;
    start_time_ = start_time;
    node_ = node;
    listener_node_ = listener_node;
    cut_off_ = cut_off;
    seamless_loop_ = seamless_loop;
    if (seamless_loop_)
        f_loop_ = true;
    sound_playing_ = false;
    reverse_ = false;

    // If no duration given use sound's duration as interval's duration
    if (duration == 0.0 && sound_)
        duration = (std::max)(sound_duration_ - start_time_, 0.0);
    _duration = duration;

    // Generate unique name if necessary
    if (name)
        _name = name.get();
    else
        _name = id;
}

void SoundInterval::loop(double start_t, double end_t, double play_rate, bool stagger)
{
    f_loop_ = true;
    CInterval::loop(start_t, end_t, play_rate);
    if (stagger)
        set_t(double(std::rand()) / double(RAND_MAX) * get_duration());
}

void SoundInterval::finish(void)
{
    in_finish_ = true;
    CInterval::finish();
    in_finish_ = false;
}

void SoundInterval::priv_initialize(double t)
{
    // If it's within a 10th of a second of the start,
    // start at the beginning.
    reverse_ = false;
    double t1 = t + start_time_;
    if (t1 < 0.1)
        t1 = 0.0;

    if (t1 < sound_duration_ && !(seamless_loop_ && sound_playing_))
    {
        ShowBase::get_global_ptr()->get_sfx_player()->play_sfx(
            sound_, f_loop_, true, volume_, t1, node_, listener_node_, cut_off_);
        sound_playing_ = true;
    }

    _state = State::S_started;
    _curr_t = t;
}

void SoundInterval::priv_instant(void)
{
    return;
}

void SoundInterval::priv_step(double t)
{
    if (_state == State::S_paused)
    {
        // Restarting from a pause.
        double t1 = t + start_time_;
        if (t1 < sound_duration_)
        {
            ShowBase::get_global_ptr()->get_sfx_player()->play_sfx(
                sound_, f_loop_, true, volume_, t1, node_, listener_node_);
        }
    }

    if (listener_node_ && !listener_node_.get().is_empty() &&
        node_ && !node_.get().is_empty())
    {
        ShowBase::get_global_ptr()->get_sfx_player()->set_final_volume(
            sound_, node_, volume_, listener_node_, cut_off_);
    }

    _state = State::S_started;
    _curr_t = t;
}

void SoundInterval::priv_finalize(void)
{
    // if we're just coming to the end of a seamlessloop, leave the sound alone,
    // let the audio subsystem loop it
    if (seamless_loop_ && sound_playing_ && f_loop_ && in_finish_)
    {
        ShowBase::get_global_ptr()->get_sfx_player()->set_final_volume(
            sound_, node_, volume_, listener_node_, cut_off_);
        return;
    }
    else if (sound_)
    {
        sound_->stop();
        sound_playing_ = false;
    }

    _curr_t = get_duration();
    _state = State::S_final;
}

void SoundInterval::priv_reverse_initialize(double t)
{
    reverse_ = true;
}

void SoundInterval::priv_reverse_instant(void)
{
    _state = State::S_initial;
}

void SoundInterval::priv_reverse_finalize(void)
{
    reverse_ = false;
    _state = State::S_initial;
}

void SoundInterval::priv_interrupt(void)
{
    if (sound_)
    {
        sound_->stop();
        sound_playing_ = false;
    }
    _state = State::S_paused;
}

}
