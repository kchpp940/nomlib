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
#ifndef NOMLIB_AUDIO_DEVICE_LOCATOR_HPP
#define NOMLIB_AUDIO_DEVICE_LOCATOR_HPP

#include <memory>

#include "nomlib/config.hpp"

namespace nom {

// Forward declarations
namespace audio {
class IAudioDevice;
class NullAudioDevice;
class IOAudioEngine;
class AudioMixer;
} // namespace audio

/// \brief Service Locator pattern implementation for audio device access.
///
/// This is the central entry point for the audio subsystem. It manages:
/// - Audio device provider registration and fallback (NullAudioDevice)
/// - AudioMixer lifecycle and bus state management
/// - IOAudioEngine registration from init_audio() / shutdown_audio()
///
/// \see http://gameprogrammingpatterns.com/service-locator.html
class AudioDeviceLocator
{
  public:
    ~AudioDeviceLocator( void );

    /// \brief Initialize the locator with NullAudioDevice as fallback.
    static void initialize( void );

    /// \brief Get the current audio device provider.
    static audio::IAudioDevice& audio_device( void );

    /// \brief Get the global AudioMixer instance for bus control.
    static audio::AudioMixer& mixer( void );

    /// \brief Look up the mixer associated with a given engine.
    ///
    /// \returns The global mixer if engine matches the registered one,
    ///          otherwise nullptr.
    static audio::AudioMixer* mixer_for_engine( audio::IOAudioEngine* engine );

    /// \brief Register an IOAudioEngine with the locator.
    ///
    /// Called by audio::init_audio() after successfully opening a device.
    /// The engine is attached to the global AudioMixer.
    static void register_engine( audio::IOAudioEngine* engine );

    /// \brief Unregister the current IOAudioEngine.
    ///
    /// Called by audio::shutdown_audio(). The global AudioMixer is detached
    /// and falls back to a null state.
    static void unregister_engine( audio::IOAudioEngine* engine );

    /// \brief Replace the audio device provider.
    ///
    /// If service is nullptr, falls back to NullAudioDevice.
    /// The locator takes ownership of the provided device.
    static void set_provider( audio::IAudioDevice* service );

  private:
    static audio::IAudioDevice* audio_;
    static audio::NullAudioDevice null_audio_;
    static audio::IAudioDevice* owned_provider_;

    static std::unique_ptr<audio::AudioMixer> mixer_;
    static audio::IOAudioEngine* active_engine_;
};

} // namespace nom

#endif // include guard defined
