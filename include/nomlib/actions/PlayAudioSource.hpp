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
#ifndef NOMLIB_ACTIONS_PLAY_AUDIO_SOURCE_HPP
#define NOMLIB_ACTIONS_PLAY_AUDIO_SOURCE_HPP

#include <memory>
#include <vector>
#include <string>

#include "nomlib/config.hpp"
#include "nomlib/actions/IActionObject.hpp"
#include "nomlib/audio/AudioMixer.hpp"

namespace nom {
namespace audio {

// Forward declarations
struct SoundBuffer;
class ISoundFileReader;
class IOAudioEngine;

} // namespace audio

/// \brief Action for playing streaming audio from a file source.
///
/// The playback is dispatched through the global AudioMixer obtained via
/// AudioDeviceLocator, ensuring bus volume (master/music/sfx/voice) is
/// consistently applied regardless of which back-end is in use.
class PlayAudioSource: public virtual IActionObject
{
  public:
    friend class ActionTest;

    typedef PlayAudioSource self_type;

    /// \brief Construct from an audio file (public API, backward compatible).
    ///
    /// \param dev      An audio engine obtained from audio::init_audio().
    ///                 The mixer used for playback is resolved through
    ///                 AudioDeviceLocator, which tracks the registered engine.
    /// \param filename Path to an audio file readable by the sound file
    ///                 reader back-end (e.g. WAV via libsndfile).
    PlayAudioSource(audio::IOAudioEngine* dev, const char* filename);

    /// \brief Construct with an explicit mixer and bus (advanced API).
    ///
    /// \param mixer    The AudioMixer to dispatch through. If nullptr the
    ///                 global mixer from AudioDeviceLocator is used.
    /// \param filename Path to an audio file.
    /// \param bus      Which audio bus to play on (defaults to SFX).
    PlayAudioSource(audio::AudioMixer* mixer, const char* filename,
                    audio::AudioBus bus = audio::AUDIO_BUS_SFX);

    virtual ~PlayAudioSource();

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

    nom::size_type curr_frame_ = 0;

    audio::AudioMixer* mixer_ = nullptr;
    audio::AudioBus bus_ = audio::AUDIO_BUS_SFX;

    std::shared_ptr<audio::ISoundFileReader> fp_;
    std::string filename_;

    typedef std::vector<audio::SoundBuffer*> audio_buffers;
    audio_buffers::iterator current_buffer_;
    audio_buffers audible_;

    uint32 input_pos_ = 0;
};

} // namespace nom

#endif // include guard defined
