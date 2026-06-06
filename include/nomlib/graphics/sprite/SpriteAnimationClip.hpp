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
#ifndef NOMLIB_GRAPHICS_SPRITE_SPRITE_ANIMATION_CLIP_HPP
#define NOMLIB_GRAPHICS_SPRITE_SPRITE_ANIMATION_CLIP_HPP

#include <string>
#include <functional>

#include "nomlib/config.hpp"

namespace nom {

/// \brief Configuration data for a single named sprite animation
///
/// \remarks A clip defines a contiguous range of frames on a sprite sheet,
/// along with playback parameters like speed, looping mode and completion
/// callback.
class SpriteAnimationClip
{
  public:
    typedef SpriteAnimationClip self_type;

    typedef std::function<void()> completion_callback;

    static const real32 DEFAULT_FPS;

    SpriteAnimationClip();

    /// \brief Construct a clip with the given frame range.
    ///
    /// \param name        Unique identifier for this clip (e.g. "idle").
    /// \param start_frame First frame index (inclusive) on the sprite sheet.
    /// \param end_frame   Last frame index (inclusive) on the sprite sheet.
    /// \param fps         Frames per second; defaults to 15.
    SpriteAnimationClip( const std::string& name,
                         int start_frame,
                         int end_frame,
                         real32 fps = DEFAULT_FPS );

    ~SpriteAnimationClip();

    SpriteAnimationClip* clone() const;

    const std::string& name() const;
    int start_frame() const;
    int end_frame() const;
    real32 fps() const;
    bool loop() const;
    bool ping_pong() const;
    const completion_callback& on_complete() const;

    /// \brief Total number of frames in this clip (absolute span).
    int num_frames() const;

    /// \brief Duration of one full play-through in seconds.
    real32 duration() const;

    void set_name( const std::string& name );
    void set_start_frame( int frame );
    void set_end_frame( int frame );
    void set_frame_range( int start, int end );
    void set_fps( real32 fps );
    void set_loop( bool enabled );
    void set_ping_pong( bool enabled );
    void set_completion_callback( const completion_callback& cb );

  private:
    std::string name_;
    int start_frame_;
    int end_frame_;
    real32 fps_;
    bool loop_;
    bool ping_pong_;
    completion_callback on_complete_;
};

} // namespace nom

#endif // include guard defined

/// \class nom::SpriteAnimationClip
/// \ingroup graphics
///
/// Typical usage:
/// \code
///   nom::SpriteAnimationClip idle("idle", 0, 3, 10.0f);
///   idle.set_loop(true);
///
///   nom::SpriteAnimationClip attack("attack", 4, 9, 24.0f);
///   attack.set_completion_callback([]{ play_sfx("swing"); });
/// \endcode
///
