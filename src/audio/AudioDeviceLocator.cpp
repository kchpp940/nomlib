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
#include "nomlib/audio/AudioDeviceLocator.hpp"

// Private headers
#include "nomlib/audio/IAudioDevice.hpp"
#include "nomlib/audio/IOAudioEngine.hpp"

namespace nom {

// Static initializations
IAudioDevice* AudioDeviceLocator::audio_ = nullptr;
NullAudioDevice AudioDeviceLocator::null_audio_;

AudioDeviceLocator::~AudioDeviceLocator( void )
{
  AudioDeviceLocator::shutdown();
}

void AudioDeviceLocator::initialize( void )
{
  if(AudioDeviceLocator::null_audio_.valid() == false) {
    AudioDeviceLocator::null_audio_.open(nullptr);
  }
  AudioDeviceLocator::audio_ = &AudioDeviceLocator::null_audio_;
}

void AudioDeviceLocator::shutdown( void )
{
  // Release the currently-installed provider if it is a heap-allocated real
  // device (never destroy the static null fallback instance that we own).
  if(AudioDeviceLocator::audio_ != nullptr &&
     AudioDeviceLocator::audio_ != &AudioDeviceLocator::null_audio_) {
    AudioDeviceLocator::audio_->close();
    NOM_DELETE_PTR(AudioDeviceLocator::audio_);
  }

  // Also reset the null fallback so its engine / mixer state and any
  // lingering source registrations are fully torn down.
  AudioDeviceLocator::null_audio_.close();
  AudioDeviceLocator::audio_ = &AudioDeviceLocator::null_audio_;
}

IAudioDevice& AudioDeviceLocator::audio_device( void )
{
  if(AudioDeviceLocator::audio_ == nullptr) {
    NOM_LOG_INFO(NOM_LOG_CATEGORY_AUDIO,
                 "AudioDevice was not yet initialized. Initializing...");
    AudioDeviceLocator::initialize();
  }

  return *AudioDeviceLocator::audio_;
}

void AudioDeviceLocator::set_provider( IAudioDevice* service )
{
  // Release the currently-installed provider before swapping (always; even the
  // null fallback gets cycled so its bus state and source registry reset).
  if(AudioDeviceLocator::audio_ != nullptr) {
    if(AudioDeviceLocator::audio_ != &AudioDeviceLocator::null_audio_) {
      AudioDeviceLocator::audio_->close();
      NOM_DELETE_PTR(AudioDeviceLocator::audio_);
    } else {
      AudioDeviceLocator::null_audio_.close();
    }
  }

  if(service == nullptr) {
    NOM_LOG_INFO(NOM_LOG_CATEGORY_APPLICATION,
                 "Audio Service given was NULL; falling back to NullAudioDevice.");
    if(AudioDeviceLocator::null_audio_.valid() == false) {
      AudioDeviceLocator::null_audio_.open(nullptr);
    }
    AudioDeviceLocator::audio_ = &AudioDeviceLocator::null_audio_;
  } else {
    AudioDeviceLocator::audio_ = service;
  }
}

audio::IOAudioEngine* AudioDeviceLocator::engine()
{
  IAudioDevice& dev = AudioDeviceLocator::audio_device();
  return dev.engine();
}

real32 AudioDeviceLocator::bus_volume(audio::AudioBus bus)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    return e->bus_volume(bus);
  }
  return audio::MIN_VOLUME;
}

void AudioDeviceLocator::set_bus_volume(audio::AudioBus bus, real32 gain)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    e->set_bus_volume(bus, gain);
  }
}

bool AudioDeviceLocator::bus_muted(audio::AudioBus bus)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    return e->bus_muted(bus);
  }
  return false;
}

void AudioDeviceLocator::set_bus_muted(audio::AudioBus bus, bool mute)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    e->set_bus_muted(bus, mute);
  }
}

bool AudioDeviceLocator::bus_paused(audio::AudioBus bus)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    return e->bus_paused(bus);
  }
  return false;
}

void AudioDeviceLocator::pause_bus(audio::AudioBus bus)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    e->pause_bus(bus);
  }
}

void AudioDeviceLocator::resume_bus(audio::AudioBus bus)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    e->resume_bus(bus);
  }
}

void AudioDeviceLocator::stop_bus(audio::AudioBus bus)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    e->stop_bus(bus);
  }
}

void AudioDeviceLocator::fade_bus_volume(audio::AudioBus bus, real32 target_gain,
                                         real32 duration)
{
  auto* e = AudioDeviceLocator::engine();
  if(e != nullptr) {
    e->fade_bus_volume(bus, target_gain, duration);
  }
}

} // namespace nom
