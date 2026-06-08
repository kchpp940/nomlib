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
#ifndef NOMLIB_AUDIO_AL_AL_AUDIO_DEVICE_WRAPPER_HPP
#define NOMLIB_AUDIO_AL_AL_AUDIO_DEVICE_WRAPPER_HPP

#include "nomlib/config.hpp"
#include "nomlib/audio/IAudioDevice.hpp"
#include "nomlib/audio/IOAudioEngine.hpp"

#include <memory>

namespace nom {
namespace audio {

struct AudioSpec;
struct ALAudioDevice;

class ALAudioDeviceWrapper: public IAudioDevice
{
  public:
    ALAudioDeviceWrapper( void );
    virtual ~ALAudioDeviceWrapper( void );

    IOAudioEngine* open( const AudioSpec* spec ) override;
    void suspend( void ) override;
    void resume( void ) override;
    void close( void ) override;

    std::string device_name( void ) const override;

  private:
    std::unique_ptr<IOAudioEngine> engine_;
    ALAudioDevice* driver_ = nullptr;
    std::string device_name_ = "OpenAL";
};

} // namespace audio
} // namespace nom

#endif // NOMLIB_AUDIO_AL_AL_AUDIO_DEVICE_WRAPPER_HPP defined
