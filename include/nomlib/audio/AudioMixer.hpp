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
#ifndef NOMLIB_AUDIO_AUDIO_MIXER_HPP
#define NOMLIB_AUDIO_AUDIO_MIXER_HPP

#include <memory>
#include <vector>
#include <algorithm>

#include "nomlib/config.hpp"
#include "nomlib/math/Point3.hpp"
#include "nomlib/audio/audio_defs.hpp"

namespace nom {
namespace audio {

// Forward declarations
class IOAudioEngine;
struct SoundBuffer;

/// \brief Audio bus type for grouping audio sources
enum AudioBus
{
  AUDIO_BUS_MASTER = 0,
  AUDIO_BUS_MUSIC,
  AUDIO_BUS_SFX,
  AUDIO_BUS_VOICE,
  AUDIO_BUS_COUNT
};

/// \brief Per-bus state: volume level and mute status
struct AudioBusState
{
  real32 volume = MAX_VOLUME;
  bool muted = false;
};

/// \brief Mixer state management for master and music/sfx/voice buses.
///
/// This class acts as the central dispatch point for audio actions, providing
/// a stable control path for music, sound effects and voice playback.
///
/// The mixer tracks all registered SoundBuffer sources and can reset() all
/// of them in one call — used during device/provider transitions to ensure
/// old sources are released along with the engine they were bound to.
class AudioMixer
{
  public:
    AudioMixer();
    explicit AudioMixer(IOAudioEngine* engine);
    ~AudioMixer();

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    /// \brief Attach a new engine to the mixer.
    ///
    /// If an engine was already attached, reset() is called first to stop
    /// and free all sources bound to the previous engine.
    void set_engine(IOAudioEngine* engine);

    IOAudioEngine* engine() const;
    bool valid() const;

    // --- Master bus controls ---

    real32 master_volume() const;
    void set_master_volume(real32 gain);
    bool master_muted() const;
    void set_master_muted(bool muted);

    // --- Per-bus controls ---

    real32 bus_volume(AudioBus bus) const;
    void set_bus_volume(AudioBus bus, real32 gain);
    bool bus_muted(AudioBus bus) const;
    void set_bus_muted(AudioBus bus, bool muted);

    // --- Effective volume (master * bus) ---

    real32 effective_volume(AudioBus bus) const;

    // --- Source control (dispatched to backend) ---

    /// \brief Play a buffer on the given bus.
    ///
    /// The buffer is automatically registered so it can be released by reset().
    void play(SoundBuffer* buffer, AudioBus bus = AUDIO_BUS_SFX);

    void stop(SoundBuffer* buffer);
    void pause(SoundBuffer* buffer);
    void resume(SoundBuffer* buffer);

    void set_source_volume(SoundBuffer* buffer, real32 gain,
                           AudioBus bus = AUDIO_BUS_SFX);
    real32 source_volume(SoundBuffer* buffer) const;

    uint32 source_state(SoundBuffer* buffer) const;

    /// \brief Push (upload) a buffer to the backend.
    ///
    /// The buffer is automatically registered so it can be released by reset().
    bool push_buffer(SoundBuffer* buffer);

    /// \brief Queue a buffer for streaming playback.
    ///
    /// The buffer is automatically registered so it can be released by reset().
    bool queue_buffer(SoundBuffer* buffer);

    /// \brief Free a single buffer and unregister it from the mixer.
    void free_buffer(SoundBuffer* buffer);

    // --- Global engine controls ---

    void suspend();
    void resume_engine();

    /// \brief Stop and free every registered source, reset all bus states
    ///        to defaults, detach the engine and mark mixer as invalid.
    ///
    /// Called by AudioDeviceLocator when the provider is detached or
    /// replaced, so no stale sources remain bound to a defunct engine.
    void reset();

    /// \brief Close the engine (called by reset()).
    void close();

  private:
    static real32 clamp_gain(real32 gain);
    void apply_bus_volume(SoundBuffer* buffer, AudioBus bus);

    /// Register a buffer with the mixer if not already tracked.
    void register_source(SoundBuffer* buffer);

    /// Unregister a buffer from the mixer.
    void unregister_source(SoundBuffer* buffer);

    /// Reset bus state to factory defaults (MAX_VOLUME, not muted).
    void reset_bus_states();

    IOAudioEngine* engine_ = nullptr;
    AudioBusState buses_[AUDIO_BUS_COUNT];
    std::vector<SoundBuffer*> sources_;
};

} // namespace audio
} // namespace nom

#endif // include guard defined
