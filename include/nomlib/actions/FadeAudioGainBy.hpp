/******************************************************************************

  nomlib - C++11 cross-platform game engine

Copyright (c) 2013, 2014, 2015, 2016 Jeffrey Carpenter <i8degrees@gmail.com> Carpenter <i8degrees@gmail.com>
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
#ifndef NOMLIB_ACTIONS_FADE_AUDIO_GAIN_BY_HPP
#define NOMLIB_ACTIONS_FADE_AUDIO_GAIN_BY_HPP

#include <memory>
#include <string>

#include "nomlib/config.hpp"
#include "nomlib/actions/IActionObject.hpp"

namespace nom {
namespace audio {

// Forward declarations
class IOAudioEngine;
struct SoundBuffer;

} // namespace audio

/// \brief Fade the audio gain (volume) of a sound buffer over time.
///
/// Two ownership modes are supported:
///   1. **File-backed** (constructed with `const char* filename`): the action
///      owns the SoundBuffer* returned by audio::create_buffer() and will
///      free it in release() via audio::free_buffer().
///   2. **Externally-supplied buffer** (constructed with `SoundBuffer*`): the
///      action is only an observer and will **never** free the buffer.
///
/// In both cases the IOAudioEngine* is an observer pointer and is never freed
/// by the action.
class FadeAudioGainBy: public virtual IActionObject
{
  public:
    /// \brief Allow access into our private parts for unit testing.
    friend class ActionTest;

    typedef FadeAudioGainBy self_type;

    /// \brief Owned-buffer constructor: load and own the audio at `filename`.
    FadeAudioGainBy(audio::IOAudioEngine* dev, const char* filename,
                    real32 delta, real32 duration);

    /// \brief Observer constructor: fade the caller-supplied buffer without
    ///        taking ownership.
    FadeAudioGainBy(audio::IOAudioEngine* dev, audio::SoundBuffer* buffer,
                    real32 delta, real32 duration);

    /// \brief Destructor.
    virtual ~FadeAudioGainBy();

    /// \brief Clone semantics follow the ownership mode of the source:
    ///        file-backed actions re-load the file; observer-mode actions share
    ///        the external buffer.
    virtual std::unique_ptr<IActionObject> clone() const override;

    virtual IActionObject::FrameState next_frame(real32 delta_time) override;

    virtual IActionObject::FrameState prev_frame(real32 delta_time) override;

    virtual void pause(real32 delta_time) override;

    virtual void resume(real32 delta_time) override;

    /// \brief Restore the volume to the value recorded at first_frame() time
    ///        and clear all frame-local state, making the action safe to replay
    ///        from the beginning (including inside RepeatFor / RepeatForever).
    virtual void rewind(real32 delta_time) override;

    /// \brief Free the owned SoundBuffer (if any) and clear all observer
    ///        pointers.  Safe to call multiple times.
    virtual void release() override;

  private:
    static const char* DEBUG_CLASS_NAME;

    IActionObject::FrameState update(real32 t, uint8 b, int16 c, real32 d);

    void first_frame(real32 delta_time);
    void last_frame(real32 delta_time);

    /// \brief Volume recorded at first_frame() time; rewind restores this.
    real32 initial_volume_;

    /// \brief The total change in the alpha blending value.
    const real32 total_displacement_;

    /// \brief Observer pointer -- never owned, never deleted.
    audio::IOAudioEngine* impl_ = nullptr;

    /// \brief When true, audible_ is freed in release() via audio::free_buffer().
    bool owns_audible_ = false;

    /// \brief Source file name; only populated when owns_audible_ is true.
    std::string source_filename_;

    /// \brief The audio buffer whose volume is being faded.
    audio::SoundBuffer* audible_ = nullptr;
};

} // namespace nom

#endif // include guard defined
