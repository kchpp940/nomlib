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
#include "nomlib/audio/AL/ALAudioDeviceWrapper.hpp"

#include "nomlib/audio/AL/ALAudioDevice.hpp"
#include "nomlib/audio/AL/ALAudioDeviceCaps.hpp"
#include "nomlib/audio/IOAudioEngine.hpp"
#include "nomlib/audio/audio_defs.hpp"
#include "nomlib/core/helpers.hpp"

namespace nom {
namespace audio {

// Forward declarations — these are implemented in ALAudioDevice.cpp
IOAudioEngine* init_openal_output(const audio::AudioSpec* request,
                                  audio::AudioSpec* spec);

ALAudioDeviceWrapper::ALAudioDeviceWrapper( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_AUDIO,
                      NOM_LOG_PRIORITY_VERBOSE );
}

ALAudioDeviceWrapper::~ALAudioDeviceWrapper( void )
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_AUDIO,
                      NOM_LOG_PRIORITY_VERBOSE );
  this->close();
}

IOAudioEngine*
ALAudioDeviceWrapper::open( const AudioSpec* spec )
{
  AudioSpec obtained_spec;
  std::memset( &obtained_spec, 0, sizeof(AudioSpec) );

  IOAudioEngine* engine = audio::init_openal_output( spec, &obtained_spec );

  if( engine != nullptr ) {
    this->engine_.reset( engine );
    if( obtained_spec.name != nullptr ) {
      this->device_name_ = obtained_spec.name;
    }

    NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                  "ALAudioDeviceWrapper: opened OpenAL device '",
                  this->device_name_.c_str(), "'" );
  } else {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_AUDIO,
                 "ALAudioDeviceWrapper::open: failed to open OpenAL device" );
  }

  return engine;
}

void ALAudioDeviceWrapper::suspend( void )
{
  if( this->engine_ != nullptr ) {
    this->engine_->suspend();
  }
}

void ALAudioDeviceWrapper::resume( void )
{
  if( this->engine_ != nullptr ) {
    this->engine_->resume();
  }
}

void ALAudioDeviceWrapper::close( void )
{
  if( this->engine_ != nullptr ) {
    NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                  "ALAudioDeviceWrapper: closing audio device" );

    this->engine_->close();
    this->engine_.reset();
  }
}

std::string ALAudioDeviceWrapper::device_name( void ) const
{
  return this->device_name_;
}

} // namespace audio
} // namespace nom
