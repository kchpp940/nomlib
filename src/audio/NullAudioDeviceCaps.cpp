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
#include "nomlib/audio/NullAudioDeviceCaps.hpp"

// Private headers
#include "nomlib/audio/audio_defs.hpp"

// FIXME(jeff): enums
#include "nomlib/audio/AL/SoundSource.hpp"

namespace nom {
namespace audio {

NullAudioEngineCaps::NullAudioEngineCaps()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_AUDIO, NOM_LOG_PRIORITY_DEBUG);
}

NullAudioEngineCaps::~NullAudioEngineCaps()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_AUDIO, NOM_LOG_PRIORITY_DEBUG);
}

void NullAudioEngineCaps::init(void* driver)
{
  (void)driver;
}

bool NullAudioEngineCaps::valid() const
{
  return false;
}

uint32 NullAudioEngineCaps::caps() const
{
  return 0;
}

void NullAudioEngineCaps::set_cap(uint32 format)
{
  (void)format;
}

bool NullAudioEngineCaps::connected() const
{
  return false;
}

uint32
NullAudioEngineCaps::channel_format(uint32 num_channels, uint32 channel_format)
{
  (void)num_channels;
  channel_format = 0;
  return channel_format;
}

bool NullAudioEngineCaps::valid_buffer(SoundBuffer* buffer)
{
  (void)buffer;
  return false;
}

bool NullAudioEngineCaps::valid_source(SoundBuffer* buffer)
{
  (void)buffer;
  return false;
}

uint32 NullAudioEngineCaps::state(SoundBuffer* buffer)
{
  (void)buffer;
  return AUDIO_STATE_STOPPED;
}

real32 NullAudioEngineCaps::pitch(SoundBuffer* buffer)
{
  (void)buffer;
  return 0.0f;
}

real32 NullAudioEngineCaps::volume() const
{
  return nom::audio::MIN_VOLUME;
}

Point3f NullAudioEngineCaps::position() const
{
  return Point3f(0.0f, 0.0f, 0.0f);
}

real32 NullAudioEngineCaps::volume(SoundBuffer* buffer) const
{
  (void)buffer;
  return nom::audio::MIN_VOLUME;
}

real32 NullAudioEngineCaps::min_volume(SoundBuffer* buffer)
{
  (void)buffer;
  return nom::audio::MIN_VOLUME;
}

real32 NullAudioEngineCaps::max_volume(SoundBuffer* buffer)
{
  (void)buffer;
  return nom::audio::MIN_VOLUME;
}

Point3f NullAudioEngineCaps::velocity(SoundBuffer* buffer)
{
  (void)buffer;
  return Point3f::zero;
}

Point3f NullAudioEngineCaps::position(SoundBuffer* buffer)
{
  (void)buffer;
  return Point3f::zero;
}

real32 NullAudioEngineCaps::playback_position(SoundBuffer* buffer)
{
  (void)buffer;
  return 0.0f;
}

real32 NullAudioEngineCaps::playback_samples(SoundBuffer* buffer)
{
  (void)buffer;
  return 0.0f;
}

void NullAudioEngineCaps::set_volume(real32 gain)
{
  (void)gain;
}

void NullAudioEngineCaps::set_position(const Point3f& p)
{
  (void)p;
}

void NullAudioEngineCaps::set_volume(SoundBuffer* target, real32 gain)
{
  (void)target;
  (void)gain;
}

void NullAudioEngineCaps::set_min_volume(SoundBuffer* target, real32 gain)
{
  (void)target;
  (void)gain;
}

void NullAudioEngineCaps::set_max_volume(SoundBuffer* target, real32 gain)
{
  (void)target;
  (void)gain;
}

void NullAudioEngineCaps::set_velocity(SoundBuffer* target, const Point3f& v)
{
  (void)target;
  (void)v;
}

void NullAudioEngineCaps::set_position(SoundBuffer* target, const Point3f& p)
{
  (void)target;
  (void)p;
}

void NullAudioEngineCaps::set_pitch(SoundBuffer* buffer, real32 pitch)
{
  (void)buffer;
  (void)pitch;
}

void NullAudioEngineCaps::set_playback_position(SoundBuffer* target,
                                                real32 offset_seconds)
{
  (void)target;
  (void)offset_seconds;
}

void NullAudioEngineCaps::play(SoundBuffer* buffer)
{
  (void)buffer;
}

void NullAudioEngineCaps::stop(SoundBuffer* buffer)
{
  (void)buffer;
}

void NullAudioEngineCaps::pause(SoundBuffer* buffer)
{
  (void)buffer;
}

void NullAudioEngineCaps::resume(SoundBuffer* buffer)
{
  (void)buffer;
}

bool NullAudioEngineCaps::push_buffer(SoundBuffer* buffer)
{
  (void)buffer;
  return false;
}

bool NullAudioEngineCaps::queue_buffer(SoundBuffer* buffer)
{
  (void)buffer;
  return false;
}

bool NullAudioEngineCaps::reset_stream_queue(SoundBuffer* buffer)
{
  (void)buffer;
  return false;
}

void NullAudioEngineCaps::suspend()
{
}

void NullAudioEngineCaps::resume()
{
}

void NullAudioEngineCaps::close()
{
}

void NullAudioEngineCaps::free_buffer(SoundBuffer* buffer)
{
  (void)buffer;
}

} // namespace audio
} // namespace nom
