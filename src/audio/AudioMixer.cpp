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
#include "nomlib/audio/AudioMixer.hpp"

#include "nomlib/audio/IOAudioEngine.hpp"
#include "nomlib/audio/SoundBuffer.hpp"
#include "nomlib/audio/AL/SoundSource.hpp"

namespace nom {
namespace audio {

AudioMixer::AudioMixer()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_AUDIO, NOM_LOG_PRIORITY_VERBOSE);
  this->reset_bus_states();
}

AudioMixer::AudioMixer(IOAudioEngine* engine)
  : engine_(engine)
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_AUDIO, NOM_LOG_PRIORITY_VERBOSE);
  this->reset_bus_states();
}

AudioMixer::~AudioMixer()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_AUDIO, NOM_LOG_PRIORITY_VERBOSE);
  this->reset();
}

void AudioMixer::set_engine(IOAudioEngine* engine)
{
  if( this->engine_ == engine ) {
    return;
  }

  if( this->engine_ != nullptr ) {
    this->reset();
  }

  this->engine_ = engine;

  if( this->engine_ != nullptr ) {
    this->reset_bus_states();
    NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                  "AudioMixer: attached to new audio engine" );
  }
}

IOAudioEngine* AudioMixer::engine() const
{
  return this->engine_;
}

bool AudioMixer::valid() const
{
  return (this->engine_ != nullptr && this->engine_->valid());
}

real32 AudioMixer::master_volume() const
{
  return this->buses_[AUDIO_BUS_MASTER].volume;
}

void AudioMixer::set_master_volume(real32 gain)
{
  this->buses_[AUDIO_BUS_MASTER].volume = clamp_gain(gain);

  if(this->valid()) {
    real32 effective = this->buses_[AUDIO_BUS_MASTER].muted ?
                        MIN_VOLUME : this->buses_[AUDIO_BUS_MASTER].volume;
    this->engine_->set_volume(effective);
  }
}

bool AudioMixer::master_muted() const
{
  return this->buses_[AUDIO_BUS_MASTER].muted;
}

void AudioMixer::set_master_muted(bool muted)
{
  this->buses_[AUDIO_BUS_MASTER].muted = muted;
  this->set_master_volume(this->buses_[AUDIO_BUS_MASTER].volume);
}

real32 AudioMixer::bus_volume(AudioBus bus) const
{
  if(bus >= 0 && bus < AUDIO_BUS_COUNT) {
    return this->buses_[bus].volume;
  }
  return MAX_VOLUME;
}

void AudioMixer::set_bus_volume(AudioBus bus, real32 gain)
{
  if(bus >= 0 && bus < AUDIO_BUS_COUNT) {
    this->buses_[bus].volume = clamp_gain(gain);
  }
}

bool AudioMixer::bus_muted(AudioBus bus) const
{
  if(bus >= 0 && bus < AUDIO_BUS_COUNT) {
    return this->buses_[bus].muted;
  }
  return false;
}

void AudioMixer::set_bus_muted(AudioBus bus, bool muted)
{
  if(bus >= 0 && bus < AUDIO_BUS_COUNT) {
    this->buses_[bus].muted = muted;
  }
}

real32 AudioMixer::effective_volume(AudioBus bus) const
{
  if(bus < 0 || bus >= AUDIO_BUS_COUNT) {
    return MIN_VOLUME;
  }

  real32 master = this->buses_[AUDIO_BUS_MASTER].muted ?
                  MIN_VOLUME : this->buses_[AUDIO_BUS_MASTER].volume;
  real32 bus_gain = this->buses_[bus].muted ?
                    MIN_VOLUME : this->buses_[bus].volume;

  real32 effective = (master / MAX_VOLUME) * (bus_gain / MAX_VOLUME) * MAX_VOLUME;
  return clamp_gain(effective);
}

void AudioMixer::play(SoundBuffer* buffer, AudioBus bus)
{
  if(this->valid() && buffer != nullptr) {
    this->register_source(buffer);
    this->apply_bus_volume(buffer, bus);
    this->engine_->play(buffer);
  }
}

void AudioMixer::stop(SoundBuffer* buffer)
{
  if(this->valid() && buffer != nullptr) {
    this->engine_->stop(buffer);
  }
}

void AudioMixer::pause(SoundBuffer* buffer)
{
  if(this->valid() && buffer != nullptr) {
    this->engine_->pause(buffer);
  }
}

void AudioMixer::resume(SoundBuffer* buffer)
{
  if(this->valid() && buffer != nullptr) {
    this->engine_->resume(buffer);
  }
}

void AudioMixer::set_source_volume(SoundBuffer* buffer, real32 gain, AudioBus bus)
{
  if(this->valid() && buffer != nullptr) {
    real32 effective = (clamp_gain(gain) / MAX_VOLUME) *
                       (this->effective_volume(bus) / MAX_VOLUME) * MAX_VOLUME;
    this->engine_->set_volume(buffer, clamp_gain(effective));
  }
}

real32 AudioMixer::source_volume(SoundBuffer* buffer) const
{
  if(this->valid() && buffer != nullptr) {
    return this->engine_->volume(buffer);
  }
  return MIN_VOLUME;
}

uint32 AudioMixer::source_state(SoundBuffer* buffer) const
{
  if(this->valid() && buffer != nullptr) {
    return this->engine_->state(buffer);
  }
  return audio::AUDIO_STATE_STOPPED;
}

bool AudioMixer::push_buffer(SoundBuffer* buffer)
{
  if(this->valid() && buffer != nullptr) {
    this->register_source(buffer);
    return this->engine_->push_buffer(buffer);
  }
  return false;
}

bool AudioMixer::queue_buffer(SoundBuffer* buffer)
{
  if(this->valid() && buffer != nullptr) {
    this->register_source(buffer);
    return this->engine_->queue_buffer(buffer);
  }
  return false;
}

void AudioMixer::free_buffer(SoundBuffer* buffer)
{
  if(buffer == nullptr) {
    return;
  }
  if(this->valid()) {
    this->engine_->free_buffer(buffer);
  }
  this->unregister_source(buffer);
}

void AudioMixer::suspend()
{
  if(this->valid()) {
    this->engine_->suspend();
  }
}

void AudioMixer::resume_engine()
{
  if(this->valid()) {
    this->engine_->resume();
  }
}

void AudioMixer::reset()
{
  NOM_LOG_INFO( NOM_LOG_CATEGORY_AUDIO,
                "AudioMixer: resetting — releasing ",
                this->sources_.size(), " source(s)" );

  for(auto* buffer : this->sources_) {
    if(buffer != nullptr && this->valid()) {
      this->engine_->stop(buffer);
      this->engine_->free_buffer(buffer);
    }
  }
  this->sources_.clear();

  this->engine_ = nullptr;
  this->reset_bus_states();
}

void AudioMixer::close()
{
  if(this->valid()) {
    this->engine_->close();
  }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

real32 AudioMixer::clamp_gain(real32 gain)
{
  if(gain < MIN_VOLUME) return MIN_VOLUME;
  if(gain > MAX_VOLUME) return MAX_VOLUME;
  return gain;
}

void AudioMixer::apply_bus_volume(SoundBuffer* buffer, AudioBus bus)
{
  real32 current = this->source_volume(buffer);
  this->set_source_volume(buffer, current, bus);
}

void AudioMixer::register_source(SoundBuffer* buffer)
{
  if(buffer == nullptr) {
    return;
  }
  auto it = std::find(this->sources_.begin(), this->sources_.end(), buffer);
  if(it == this->sources_.end()) {
    this->sources_.push_back(buffer);
  }
}

void AudioMixer::unregister_source(SoundBuffer* buffer)
{
  if(buffer == nullptr) {
    return;
  }
  auto it = std::find(this->sources_.begin(), this->sources_.end(), buffer);
  if(it != this->sources_.end()) {
    this->sources_.erase(it);
  }
}

void AudioMixer::reset_bus_states()
{
  for(int i = 0; i < AUDIO_BUS_COUNT; ++i) {
    this->buses_[i].volume = MAX_VOLUME;
    this->buses_[i].muted = false;
  }
}

} // namespace audio
} // namespace nom
