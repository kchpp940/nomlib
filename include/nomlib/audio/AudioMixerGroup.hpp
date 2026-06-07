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
#ifndef NOMLIB_AUDIO_AUDIO_MIXER_GROUP_HPP
#define NOMLIB_AUDIO_AUDIO_MIXER_GROUP_HPP

#include <array>
#include <vector>
#include <functional>

#include "nomlib/config.hpp"
#include "nomlib/audio/audio_defs.hpp"

namespace nom {
namespace audio {

// Forward declarations
struct SoundBuffer;

/// \brief Independent audio bus / mixer group management layer.
///
/// This class owns the authoritative state of Master/Music/Sfx/Voice buses,
/// including per-bus volume, mute and pause flags. It also tracks which
/// SoundBuffer sources have been registered to each bus and computes the
/// effective (Master-cascaded) gain that should be applied to a source.
///
/// IOAudioEngine back-ends (ALAudioEngine, NullAudioEngineCaps) hold an
/// instance of this class and forward bus queries to it. The engine is
/// responsible for applying the computed gain to its underlying audio
/// sources (e.g. by calling alSourcef(..., AL_GAIN, ...) for OpenAL) and
/// for performing pause/resume/stop on the real sources when the mixer
/// requests it via the SourceCallback.
class AudioMixerGroup
{
  public:
    typedef std::function<void(SoundBuffer*)> SourceCallback;

    AudioMixerGroup();
    ~AudioMixerGroup();

    /// \brief Set optional callbacks invoked by pause/resume/stop_bus.
    ///
    /// The engine wires these to its own platform-specific pause/resume/stop
    /// implementation for a single SoundBuffer. If a callback is empty the
    /// mixer simply skips calling it.
    void set_pause_callback(SourceCallback cb);
    void set_resume_callback(SourceCallback cb);
    void set_stop_callback(SourceCallback cb);
    void set_gain_callback(SourceCallback cb);

    // -- Per-bus state --------------------------------------------------

    real32 bus_volume(AudioBus bus) const;
    void set_bus_volume(AudioBus bus, real32 gain);

    bool bus_muted(AudioBus bus) const;
    void set_bus_muted(AudioBus bus, bool mute);

    bool bus_paused(AudioBus bus) const;
    void pause_bus(AudioBus bus);
    void resume_bus(AudioBus bus);
    void stop_bus(AudioBus bus);

    /// \brief Immediately jump a bus to a target gain.
    ///
    /// Real fade animations are driven by the FadeAudioGainBy action; this
    /// low-level call just re-applies the bus gain and is also used as the
    /// default no-op back-end for NullAudioEngineCaps.
    void fade_bus_volume(AudioBus bus, real32 target_gain, real32 duration);

    const AudioBusStates& bus_states() const;

    // -- Source registry ------------------------------------------------

    /// \brief Register a source with its assigned bus.
    ///
    /// Safe to call multiple times for the same buffer; duplicates are
    /// ignored.
    void register_source(SoundBuffer* target);

    /// \brief Remove a source from whichever bus it was registered to.
    void unregister_source(SoundBuffer* target);

    // -- Gain computation -----------------------------------------------

    /// \brief Compute the effective linear volume for a bus, cascading the
    /// Master bus gain and respecting mute flags on both Master and the
    /// target bus.
    ///
    /// The returned value is in the 0..100 range used throughout nomlib's
    /// public audio API.
    real32 effective_bus_gain(AudioBus bus) const;

  private:
    void apply_gain_to_bus(AudioBus bus);
    void apply_gain_to_all();

    AudioBusStates states_;

    typedef std::vector<SoundBuffer*> SourceList;
    typedef std::array<SourceList, AUDIO_BUS_COUNT> BusSources;
    BusSources sources_;

    SourceCallback pause_cb_;
    SourceCallback resume_cb_;
    SourceCallback stop_cb_;
    SourceCallback gain_cb_;
};

} // namespace audio
} // namespace nom

#endif // include guard defined
