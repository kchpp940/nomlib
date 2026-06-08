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

namespace nom {

// Static initializations
audio::IAudioDevice* AudioDeviceLocator::audio_ = nullptr;
audio::NullAudioDevice AudioDeviceLocator::null_audio_;
audio::IAudioDevice* AudioDeviceLocator::owned_provider_ = nullptr;

std::unique_ptr<audio::AudioMixer> AudioDeviceLocator::mixer_;
audio::IOAudioEngine* AudioDeviceLocator::active_engine_ = nullptr;

AudioDeviceLocator::~AudioDeviceLocator( void )
{
  NOM_DELETE_PTR( AudioDeviceLocator::owned_provider_ );
  AudioDeviceLocator::mixer_.reset();
  AudioDeviceLocator::active_engine_ = nullptr;
}

void AudioDeviceLocator::initialize( void )
{
  AudioDeviceLocator::audio_ = &AudioDeviceLocator::null_audio_;

  if( AudioDeviceLocator::mixer_ == nullptr ) {
    AudioDeviceLocator::mixer_.reset( new audio::AudioMixer() );
  }
  AudioDeviceLocator::active_engine_ = nullptr;
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
    AudioDeviceLocator::initialize();
  }
  return *AudioDeviceLocator::mixer_;
}

audio::AudioMixer*
AudioDeviceLocator::mixer_for_engine( audio::IOAudioEngine* engine )
{
  if( AudioDeviceLocator::mixer_ == nullptr ) {
    AudioDeviceLocator::initialize();
  }
  if( engine == nullptr || engine == AudioDeviceLocator::active_engine_ ) {
    return AudioDeviceLocator::mixer_.get();
  }
  return nullptr;
}

void AudioDeviceLocator::register_engine( audio::IOAudioEngine* engine )
{
  if( AudioDeviceLocator::mixer_ == nullptr ) {
    AudioDeviceLocator::initialize();
  }
  if( engine != nullptr ) {
    AudioDeviceLocator::mixer_->set_engine( engine );
    AudioDeviceLocator::active_engine_ = engine;

    NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                  "AudioDeviceLocator: registered audio engine with mixer" );
  }
}

void AudioDeviceLocator::unregister_engine( audio::IOAudioEngine* engine )
{
  if( AudioDeviceLocator::mixer_ == nullptr ) {
    return;
  }
  if( engine == nullptr || engine == AudioDeviceLocator::active_engine_ ) {
    AudioDeviceLocator::mixer_->close();
    AudioDeviceLocator::mixer_->set_engine( nullptr );
    AudioDeviceLocator::active_engine_ = nullptr;

    NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                  "AudioDeviceLocator: unregistered audio engine from mixer" );
  }
}

void AudioDeviceLocator::set_provider( audio::IAudioDevice* service )
{
  NOM_DELETE_PTR( AudioDeviceLocator::owned_provider_ );

  if( service == nullptr )
  {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_APPLICATION, "Audio Service given was NULL; initializing NullAudioDevice..." );
    AudioDeviceLocator::initialize();
  }
  else
  {
    AudioDeviceLocator::audio_ = service;
    AudioDeviceLocator::owned_provider_ = service;
  }
}

} // namespace nom
