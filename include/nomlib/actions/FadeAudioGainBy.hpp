/******************************************************************************

  nomlib - C++11 cross-platform game engine

Copyright (c) 2013, 2014, 2015, 2016 Jeffrey Carpenter <i8degrees@gmail.com>
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
#include "nomlib/audio/AudioMixer.hpp"

namespace nom {
namespace audio {

// Forward declarations
struct SoundBuffer;

} // namespace audio

/// \brief Action for fading audio gain (volume) by a delta value over time.
///
/// This action dispatches volume changes through AudioMixer, ensuring
/// proper bus volume application for stable music/sfx/voice control paths.
class FadeAudioGainBy: public virtual IActionObject
{
  public:
    friend class ActionTest;

    typedef FadeAudioGainBy self_type;

    /// \brief Construct from an audio file, using the default SFX bus.
    FadeAudioGainBy(audio::IOAudioEngine* dev, const char* filename,
                    real32 delta, real32 duration);

    /// \brief Construct from a pre-initialized audio buffer, default SFX bus.
    FadeAudioGainBy(audio::IOAudioEngine* dev, audio::SoundBuffer* buffer,
                    real32 delta, real32 duration);

    /// \brief Construct from an audio file with a specific mixer bus.
    FadeAudioGainBy(audio::AudioMixer* mixer, const char* filename,
                    real32 delta, real32 duration,
                    audio::AudioBus bus = audio::AUDIO_BUS_SFX);

    /// \brief Construct from a pre-initialized audio buffer with a specific bus.
    FadeAudioGainBy(audio::AudioMixer* mixer, audio::SoundBuffer* buffer,
                    real32 delta, real32 duration,
                    audio::AudioBus bus = audio::AUDIO_BUS_SFX);

    virtual ~FadeAudioGainBy();

    virtual std::unique_ptr<IActionObject> clone() const override;

    virtual IActionObject::FrameState next_frame(real32 delta_time) override;

    virtual IActionObject::FrameState prev_frame(real32 delta_time) override;

    virtual void pause(real32 delta_time) override;

    virtual void resume(real32 delta_time) override;

    virtual void rewind(real32 delta_time) override;

    virtual void release() override;

  private:
    static const char* DEBUG_CLASS_NAME;

    IActionObject::FrameState update(real32 t, uint8 b, int16 c, real32 d);

    void first_frame(real32 delta_time);
    void last_frame(real32 delta_time);

    real32 initial_volume_;

    const real32 total_displacement_;

    audio::AudioMixer* mixer_ = nullptr;
    std::shared_ptr<audio::AudioMixer> owned_mixer_;
    audio::AudioBus bus_ = audio::AUDIO_BUS_SFX;
    std::string filename_;

    bool owns_buffer_ = false;
    audio::SoundBuffer* audible_ = nullptr;
};

} // namespace nom

#endif // include guard defined
