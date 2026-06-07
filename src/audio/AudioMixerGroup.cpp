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
#include "nomlib/audio/AudioMixerGroup.hpp"

#include <algorithm>

// Private headers
#include "nomlib/audio/SoundBuffer.hpp"
#include "nomlib/math/math_helpers.hpp"

namespace nom {
namespace audio {

AudioMixerGroup::AudioMixerGroup()
  : states_(AUDIO_BUS_COUNT)
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_AUDIO, NOM_LOG_PRIORITY_DEBUG);
}

AudioMixerGroup::~AudioMixerGroup()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_AUDIO, NOM_LOG_PRIORITY_DEBUG);
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void AudioMixerGroup::set_pause_callback(SourceCallback cb)
{
  this->pause_cb_ = std::move(cb);
}

void AudioMixerGroup::set_resume_callback(SourceCallback cb)
{
  this->resume_cb_ = std::move(cb);
}

void AudioMixerGroup::set_stop_callback(SourceCallback cb)
{
  this->stop_cb_ = std::move(cb);
}

void AudioMixerGroup::set_gain_callback(SourceCallback cb)
{
  this->gain_cb_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// Per-bus state
// ---------------------------------------------------------------------------

real32 AudioMixerGroup::bus_volume(AudioBus bus) const
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx < this->states_.size()) {
    return this->states_[idx].volume;
  }
  return MIN_VOLUME;
}

void AudioMixerGroup::set_bus_volume(AudioBus bus, real32 gain)
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx >= this->states_.size()) {
    return;
  }

  if(gain < MIN_VOLUME) gain = MIN_VOLUME;
  if(gain > MAX_VOLUME) gain = MAX_VOLUME;
  this->states_[idx].volume = gain;

  if(bus == AudioBus::Master) {
    this->apply_gain_to_all();
  } else {
    this->apply_gain_to_bus(bus);
  }
}

bool AudioMixerGroup::bus_muted(AudioBus bus) const
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx < this->states_.size()) {
    return this->states_[idx].muted;
  }
  return false;
}

void AudioMixerGroup::set_bus_muted(AudioBus bus, bool mute)
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx >= this->states_.size()) {
    return;
  }

  this->states_[idx].muted = mute;

  if(bus == AudioBus::Master) {
    this->apply_gain_to_all();
  } else {
    this->apply_gain_to_bus(bus);
  }
}

bool AudioMixerGroup::bus_paused(AudioBus bus) const
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx < this->states_.size()) {
    return this->states_[idx].paused;
  }
  return false;
}

void AudioMixerGroup::pause_bus(AudioBus bus)
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx >= this->states_.size() || !this->pause_cb_) {
    return;
  }

  this->states_[idx].paused = true;

  if(bus == AudioBus::Master) {
    for(auto& list : this->sources_) {
      for(auto* src : list) {
        if(src != nullptr) {
          this->pause_cb_(src);
        }
      }
    }
  } else {
    for(auto* src : this->sources_[idx]) {
      if(src != nullptr) {
        this->pause_cb_(src);
      }
    }
  }
}

void AudioMixerGroup::resume_bus(AudioBus bus)
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx >= this->states_.size() || !this->resume_cb_) {
    return;
  }

  this->states_[idx].paused = false;

  auto master_idx = static_cast<nom::size_type>(AudioBus::Master);
  bool master_paused =
    (master_idx < this->states_.size()) ?
      this->states_[master_idx].paused : false;

  if(bus == AudioBus::Master) {
    for(auto& list : this->sources_) {
      for(auto* src : list) {
        if(src != nullptr) {
          this->resume_cb_(src);
        }
      }
    }
  } else if(!master_paused) {
    for(auto* src : this->sources_[idx]) {
      if(src != nullptr) {
        this->resume_cb_(src);
      }
    }
  }
}

void AudioMixerGroup::stop_bus(AudioBus bus)
{
  auto idx = static_cast<nom::size_type>(bus);
  if(idx >= this->states_.size() || !this->stop_cb_) {
    return;
  }

  if(bus == AudioBus::Master) {
    for(auto& list : this->sources_) {
      for(auto* src : list) {
        if(src != nullptr) {
          this->stop_cb_(src);
        }
      }
    }
  } else {
    for(auto* src : this->sources_[idx]) {
      if(src != nullptr) {
        this->stop_cb_(src);
      }
    }
  }
}

void AudioMixerGroup::fade_bus_volume(AudioBus bus, real32 target_gain,
                                     real32 duration)
{
  // Low-level driver: just snap to the target gain. Real tweening is handled
  // by the FadeAudioGainBy action which calls set_bus_volume each frame.
  (void)duration;
  this->set_bus_volume(bus, target_gain);
}

const AudioBusStates& AudioMixerGroup::bus_states() const
{
  return this->states_;
}

// ---------------------------------------------------------------------------
// Source registry
// ---------------------------------------------------------------------------

void AudioMixerGroup::register_source(SoundBuffer* target)
{
  if(target == nullptr) {
    return;
  }

  auto idx = static_cast<nom::size_type>(target->bus);
  if(idx >= this->sources_.size()) {
    return;
  }

  auto& list = this->sources_[idx];
  auto it = std::find(list.begin(), list.end(), target);
  if(it == list.end()) {
    list.push_back(target);
  }
}

void AudioMixerGroup::unregister_source(SoundBuffer* target)
{
  if(target == nullptr) {
    return;
  }

  for(auto& list : this->sources_) {
    auto it = std::find(list.begin(), list.end(), target);
    if(it != list.end()) {
      list.erase(it);
    }
  }
}

// ---------------------------------------------------------------------------
// Gain computation
// ---------------------------------------------------------------------------

real32 AudioMixerGroup::effective_bus_gain(AudioBus bus) const
{
  auto bus_idx = static_cast<nom::size_type>(bus);
  if(bus_idx >= this->states_.size()) {
    return MIN_VOLUME;
  }

  real32 master_gain = MAX_VOLUME;
  auto master_idx = static_cast<nom::size_type>(AudioBus::Master);
  if(master_idx < this->states_.size()) {
    if(this->states_[master_idx].muted) {
      master_gain = MIN_VOLUME;
    } else {
      master_gain = this->states_[master_idx].volume;
    }
  }

  real32 bus_gain = this->states_[bus_idx].volume;
  if(this->states_[bus_idx].muted) {
    bus_gain = MIN_VOLUME;
  }

  return (master_gain * bus_gain) / MAX_VOLUME;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void AudioMixerGroup::apply_gain_to_bus(AudioBus bus)
{
  if(!this->gain_cb_) {
    return;
  }

  auto idx = static_cast<nom::size_type>(bus);
  if(idx >= this->sources_.size()) {
    return;
  }

  for(auto* src : this->sources_[idx]) {
    if(src != nullptr) {
      this->gain_cb_(src);
    }
  }
}

void AudioMixerGroup::apply_gain_to_all()
{
  if(!this->gain_cb_) {
    return;
  }

  for(auto& list : this->sources_) {
    for(auto* src : list) {
      if(src != nullptr) {
        this->gain_cb_(src);
      }
    }
  }
}

} // namespace audio
} // namespace nom
