/******************************************************************************

  nomlib - C++11 cross-platform game engine

Copyright (c) 2013, 2014 Jeffrey Carpenter <i8degrees@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
#ifndef NOMLIB_GRAPHICS_SPRITE_SPRITE_ANIMATOR_HPP
#define NOMLIB_GRAPHICS_SPRITE_SPRITE_ANIMATOR_HPP

#include <string>
#include <map>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/Timer.hpp"
#include "nomlib/graphics/sprite/SpriteAnimationClip.hpp"

namespace nom {

// Forward declarations
class SpriteBatch;
class Value;

/// \brief Playback controller for named sprite animations on a SpriteBatch
///
/// \remarks SpriteAnimator manages a collection of SpriteAnimationClip objects
/// and drives frame selection on an attached SpriteBatch. It can be updated
/// directly from your game loop, or driven through the actions system via
/// nom::SpriteAnimatorAction.
class SpriteAnimator
{
  public:
    typedef SpriteAnimator self_type;

    /// \brief Playback state of the animator.
    enum State
    {
      STOPPED = 0,
      PLAYING,
      PAUSED,
    };

    SpriteAnimator();

    /// \brief Construct an animator bound to an existing SpriteBatch.
    explicit SpriteAnimator( SpriteBatch& drawable );

    ~SpriteAnimator();

    SpriteAnimator* clone() const;

    /// \brief Attach a SpriteBatch to receive frame updates.
    void set_target( SpriteBatch& drawable );

    /// \brief Register a clip.
    ///
    /// \returns Boolean FALSE if a clip with the same name already exists.
    bool add_clip( const SpriteAnimationClip& clip );

    /// \brief Register a set of clips from a deserialized JSON object.
    ///
    /// Expected JSON structure:
    /// \code
    /// {
    ///   "animations": {
    ///     "idle":    { "start": 0, "end": 3, "fps": 10, "loop": true },
    ///     "walk":    { "start": 4, "end": 11, "fps": 15, "loop": true },
    ///     "attack":  { "start": 12, "end": 17, "fps": 24 }
    ///   }
    /// }
    /// \endcode
    bool load_clips( const Value& object );

    /// \brief Remove a previously registered clip.
    bool remove_clip( const std::string& name );

    /// \brief Remove all registered clips.
    void remove_clips();

    /// \brief Number of registered clips.
    nom::size_type num_clips() const;

    /// \brief Look up a clip by name (read-only access).
    const SpriteAnimationClip* clip( const std::string& name ) const;

    /// \brief Begin (or restart) playback of a named clip.
    ///
    /// \returns Boolean FALSE if no such clip exists.
    bool play( const std::string& name );

    /// \brief Pause playback at the current frame.
    void pause();

    /// \brief Resume playback from the last paused position.
    void resume();

    /// \brief Stop playback and reset to the first frame of the current clip.
    void stop();

    /// \brief Advance the animation by delta_time seconds.
    ///
    /// \returns One of SpriteAnimator::State enumeration values.
    State update( real32 delta_time );

    State state() const;
    const std::string& current_clip_name() const;
    int current_frame() const;
    bool playing() const;
    bool paused() const;
    bool stopped() const;

  private:
    void reset_playback_state();

    int compute_sheet_frame() const;

    void fire_completion_if_needed();

    typedef std::map<std::string, SpriteAnimationClip> clip_container;
    typedef clip_container::const_iterator clip_iterator;

    SpriteBatch* drawable_;

    clip_container clips_;

    State state_;
    clip_iterator current_clip_;
    std::string current_clip_name_;

    real32 elapsed_time_;
    int logical_frame_;
    int direction_;
    bool completion_fired_;

    Timer timer_;
};

} // namespace nom

#endif // include guard defined

/// \class nom::SpriteAnimator
/// \ingroup graphics
///
/// Typical usage with manual updates:
/// \code
///   nom::SpriteBatch sprite;
///   nom::SpriteAnimator anim(sprite);
///
///   anim.add_clip(nom::SpriteAnimationClip("idle", 0, 3, 10.0f).set_loop(true));
///   anim.add_clip(nom::SpriteAnimationClip("walk", 4, 11, 15.0f).set_loop(true));
///   anim.add_clip(nom::SpriteAnimationClip("attack", 12, 17, 24.0f));
///
///   anim.play("idle");
///
///   // game loop
///   while(running) {
///     anim.update(delta_time);
///     // ...
///   }
/// \endcode
///
