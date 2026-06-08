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
struct AudioSpec;
} // namespace audio

/// \brief Service Locator pattern implementation for audio device access.
///
/// This is the **sole** entry point for the audio subsystem lifecycle.
/// Provider attach / detach, engine creation / destruction, mixer reset /
/// rebinding and source teardown all happen here — no other code path should
/// directly manipulate the active engine or mixer.
///
/// Lifecycle contract:
/// 1. `initialize()` installs the NullAudioDevice fallback.
/// 2. `set_provider(new_device, spec)` detaches the current provider, resets
///    the mixer (stops + frees every source, resets bus state), then opens
///    the new device and binds its engine to the mixer.
/// 3. `set_provider(nullptr, nullptr)` falls back to NullAudioDevice.
/// 4. `mixer_for_engine(engine)` returns the global mixer **only** if engine
///    matches the currently active engine — otherwise nullptr. Callers must
///    not silently fall back to a different engine.
///
/// \see http://gameprogrammingpatterns.com/service-locator.html
class AudioDeviceLocator
{
  public:
    ~AudioDeviceLocator( void );

    /// \brief Initialize the locator with the NullAudioDevice fallback.
    static void initialize( void );

    /// \brief Get the current audio device provider.
    static audio::IAudioDevice& audio_device( void );

    /// \brief Get the global AudioMixer bound to the active engine.
    ///
    /// \note If no provider is attached the mixer exists but has no engine
    ///       and `valid()` returns false.
    static audio::AudioMixer& mixer( void );

    /// \brief Look up the mixer for a given engine pointer.
    ///
    /// \returns The global mixer if \p engine matches the currently active
    ///          engine registered via set_provider. Returns nullptr if the
    ///          engine does not match — callers **must** treat a nullptr as
    ///          a hard error and not fall back to the global mixer, as doing
    ///          so would route sources to the wrong device.
    static audio::AudioMixer* mixer_for_engine( audio::IOAudioEngine* engine );

    /// \brief Get the currently active engine (may be nullptr).
    static audio::IOAudioEngine* active_engine( void );

    /// \brief Replace the audio device provider.
    ///
    /// This is the only valid way to change the audio device. It performs a
    /// complete teardown of the previous provider before bringing up the new
    /// one:
    ///   1. Mixer::reset() — stops + frees all tracked sources, resets bus
    ///      state to defaults and detaches the old engine.
    ///   2. Old IAudioDevice::close() + delete (if owned).
    ///   3. If \p device is nullptr the NullAudioDevice fallback is installed
    ///      with a null spec.
    ///   4. New IAudioDevice::open(spec) → IOAudioEngine*.
    ///   5. AudioMixer::set_engine(engine) binds the mixer to the new engine.
    ///
    /// The locator takes ownership of \p device and will delete it on the
    /// next set_provider() call or in the destructor.
    static void set_provider( audio::IAudioDevice* device,
                              const audio::AudioSpec* spec );

  private:
    /// \brief Detach the current provider: reset mixer, close device.
    static void detach_current_provider( void );

    /// \brief Attach a provider: open(spec), bind engine to mixer.
    static void attach_provider( audio::IAudioDevice* device,
                                 const audio::AudioSpec* spec );

    static audio::IAudioDevice* audio_;
    static audio::NullAudioDevice null_audio_;
    static audio::IAudioDevice* owned_provider_;

    static std::unique_ptr<audio::AudioMixer> mixer_;
    static audio::IOAudioEngine* active_engine_;
};

} // namespace nom

#endif // include guard defined
