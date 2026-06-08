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

#include "nomlib/audio/IAudioDevice.hpp"
#include "nomlib/audio/NullAudioDevice.hpp"
#include "nomlib/audio/IOAudioEngine.hpp"
#include "nomlib/audio/AudioMixer.hpp"
#include "nomlib/audio/audio_defs.hpp"

namespace nom {

// Static initializations
audio::IAudioDevice* AudioDeviceLocator::audio_ = nullptr;
audio::NullAudioDevice AudioDeviceLocator::null_audio_;
audio::IAudioDevice* AudioDeviceLocator::owned_provider_ = nullptr;

std::unique_ptr<audio::AudioMixer> AudioDeviceLocator::mixer_;
audio::IOAudioEngine* AudioDeviceLocator::active_engine_ = nullptr;

AudioDeviceLocator::~AudioDeviceLocator( void )
{
  AudioDeviceLocator::detach_current_provider();
  AudioDeviceLocator::mixer_.reset();
}

void AudioDeviceLocator::initialize( void )
{
  if( AudioDeviceLocator::mixer_ == nullptr ) {
    AudioDeviceLocator::mixer_.reset( new audio::AudioMixer() );
  }

  AudioDeviceLocator::set_provider( nullptr, nullptr );
}

audio::IAudioDevice& AudioDeviceLocator::audio_device( void )
{
  if( AudioDeviceLocator::audio_ == nullptr ) {
    AudioDeviceLocator::initialize();
  }
  return *AudioDeviceLocator::audio_;
}

audio::AudioMixer& AudioDeviceLocator::mixer( void )
{
  if( AudioDeviceLocator::mixer_ == nullptr ) {
    AudioDeviceLocator::mixer_.reset( new audio::AudioMixer() );
  }
  return *AudioDeviceLocator::mixer_;
}

audio::AudioMixer*
AudioDeviceLocator::mixer_for_engine( audio::IOAudioEngine* engine )
{
  if( AudioDeviceLocator::mixer_ == nullptr ) {
    AudioDeviceLocator::mixer_.reset( new audio::AudioMixer() );
  }

  if( engine == AudioDeviceLocator::active_engine_ ) {
    return AudioDeviceLocator::mixer_.get();
  }

  NOM_LOG_WARN( NOM_LOG_CATEGORY_AUDIO,
                "AudioDeviceLocator::mixer_for_engine: engine mismatch — "
                "caller engine does not match active engine registered with locator" );
  return nullptr;
}

audio::IOAudioEngine* AudioDeviceLocator::active_engine( void )
{
  return AudioDeviceLocator::active_engine_;
}

void AudioDeviceLocator::set_provider( audio::IAudioDevice* device,
                                       const audio::AudioSpec* spec )
{
  AudioDeviceLocator::detach_current_provider();

  if( device == nullptr ) {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_APPLICATION,
                  "AudioDeviceLocator: falling back to NullAudioDevice" );
    AudioDeviceLocator::attach_provider( &AudioDeviceLocator::null_audio_, nullptr );
  } else {
    AudioDeviceLocator::owned_provider_ = device;
    AudioDeviceLocator::attach_provider( device, spec );
  }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void AudioDeviceLocator::detach_current_provider( void )
{
  if( AudioDeviceLocator::mixer_ != nullptr ) {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                  "AudioDeviceLocator: detaching current provider" );
    AudioDeviceLocator::mixer_->reset();
  }

  if( AudioDeviceLocator::audio_ != nullptr ) {
    AudioDeviceLocator::audio_->close();
  }

  NOM_DELETE_PTR( AudioDeviceLocator::owned_provider_ );

  AudioDeviceLocator::audio_ = nullptr;
  AudioDeviceLocator::active_engine_ = nullptr;
}

void AudioDeviceLocator::attach_provider( audio::IAudioDevice* device,
                                          const audio::AudioSpec* spec )
{
  NOM_ASSERT( device != nullptr );
  if( device == nullptr ) {
    return;
  }

  AudioDeviceLocator::audio_ = device;

  audio::IOAudioEngine* engine = device->open( spec );

  if( engine != nullptr && AudioDeviceLocator::mixer_ != nullptr ) {
    AudioDeviceLocator::mixer_->set_engine( engine );
    AudioDeviceLocator::active_engine_ = engine;

    NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                  "AudioDeviceLocator: attached provider '",
                  device->device_name(), "' — engine bound to mixer" );
  } else {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_AUDIO,
                  "AudioDeviceLocator: attach_provider got null engine from device open()" );
  }
}

} // namespace nom
