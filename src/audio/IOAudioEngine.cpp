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
#include "nomlib/audio/IOAudioEngine.hpp"

// Private headers
#include "nomlib/audio/AudioMixerGroup.hpp"

namespace nom {
namespace audio {

AudioMixerGroup* IOAudioEngine::mixer()
{
  return nullptr;
}

const AudioMixerGroup* IOAudioEngine::mixer() const
{
  return nullptr;
}

// -- Convenience accessors ----------------------------------------------------
//
// These are thin, safe forwards. If a back-end does not provide an
// AudioMixerGroup (mixer() returns nullptr), we behave as a no-op and return
// sane defaults so existing call sites don't need null checks everywhere.

real32 IOAudioEngine::bus_volume(AudioBus bus) const
{
  auto* m = this->mixer();
  if(m != nullptr) {
    return m->bus_volume(bus);
  }
  return MAX_VOLUME;
}

void IOAudioEngine::set_bus_volume(AudioBus bus, real32 gain)
{
  auto* m = this->mixer();
  if(m != nullptr) {
    m->set_bus_volume(bus, gain);
  }
}

bool IOAudioEngine::bus_muted(AudioBus bus) const
{
  auto* m = this->mixer();
  if(m != nullptr) {
    return m->bus_muted(bus);
  }
  return false;
}

void IOAudioEngine::set_bus_muted(AudioBus bus, bool mute)
{
  auto* m = this->mixer();
  if(m != nullptr) {
    m->set_bus_muted(bus, mute);
  }
}

bool IOAudioEngine::bus_paused(AudioBus bus) const
{
  auto* m = this->mixer();
  if(m != nullptr) {
    return m->bus_paused(bus);
  }
  return false;
}

void IOAudioEngine::pause_bus(AudioBus bus)
{
  auto* m = this->mixer();
  if(m != nullptr) {
    m->pause_bus(bus);
  }
}

void IOAudioEngine::resume_bus(AudioBus bus)
{
  auto* m = this->mixer();
  if(m != nullptr) {
    m->resume_bus(bus);
  }
}

void IOAudioEngine::stop_bus(AudioBus bus)
{
  auto* m = this->mixer();
  if(m != nullptr) {
    m->stop_bus(bus);
  }
}

void IOAudioEngine::fade_bus_volume(AudioBus bus, real32 target_gain,
                                    real32 duration)
{
  auto* m = this->mixer();
  if(m != nullptr) {
    m->fade_bus_volume(bus, target_gain, duration);
  }
}

} // namespace audio
} // namespace nom
