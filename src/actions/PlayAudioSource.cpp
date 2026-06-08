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
#include "nomlib/actions/PlayAudioSource.hpp"

// Private headers
#include "nomlib/core/unique_ptr.hpp"
#include "nomlib/math/math_helpers.hpp"
#include "nomlib/audio/audio_defs.hpp"
#include "nomlib/audio/IOAudioEngine.hpp"

// Forward declarations
#include "nomlib/audio/libsndfile/SoundFileReader.hpp"
#include "nomlib/audio/SoundBuffer.hpp"
#include "nomlib/audio/AL/SoundSource.hpp"

namespace nom {

const char* PlayAudioSource::DEBUG_CLASS_NAME = "[PlayAudioSource]:";

PlayAudioSource::
PlayAudioSource(audio::IOAudioEngine* dev, const char* filename)
  : mixer_(nullptr)
  , bus_(audio::AUDIO_BUS_SFX)
  , filename_(filename ? filename : "")
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);

  this->owned_mixer_.reset(new audio::AudioMixer(dev));
  this->mixer_ = this->owned_mixer_.get();

  audio::SoundBuffer* buffer = nullptr;
  audio::SoundInfo metadata = {};
  this->elapsed_frames_ = 0.0f;

  this->fp_.reset(new audio::SoundFileReader());
  NOM_ASSERT(this->fp_ != nullptr);
  if(this->fp_ != nullptr) {

    if(this->fp_->open(filename, metadata) == false) {
      return;
    }

    if(this->fp_->valid() == false) {
      return;
    }

    auto samples_per_second = metadata.sample_rate;
    auto num_channels = metadata.channel_count;
    auto channel_format = metadata.channel_format;

    buffer =
      audio::create_buffer_memory(samples_per_second, num_channels,
                                  channel_format);

    for(auto buffer_idx = 0;
        buffer_idx != audio::TOTAL_NUM_BUFFERS;
        ++buffer_idx)
    {
      if(buffer == nullptr) {
        break;
      }

      if(audio::write_info(buffer, metadata) == false) {
        return;
      }

      this->set_duration(buffer->duration);
      this->audible_.push_back(buffer);

    } // end for TOTAL_NUM_BUFFERS loop

    this->current_buffer_ = this->audible_.begin();
  }
  NOM_DUMP(this->audible_.size());

  this->input_pos_ = 0;
}

PlayAudioSource::
PlayAudioSource(audio::AudioMixer* mixer, const char* filename,
                audio::AudioBus bus)
  : mixer_(mixer)
  , bus_(bus)
  , filename_(filename ? filename : "")
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);

  audio::SoundBuffer* buffer = nullptr;
  audio::SoundInfo metadata = {};
  this->elapsed_frames_ = 0.0f;

  this->fp_.reset(new audio::SoundFileReader());
  NOM_ASSERT(this->fp_ != nullptr);
  if(this->fp_ != nullptr) {

    if(this->fp_->open(filename, metadata) == false) {
      return;
    }

    if(this->fp_->valid() == false) {
      return;
    }

    auto samples_per_second = metadata.sample_rate;
    auto num_channels = metadata.channel_count;
    auto channel_format = metadata.channel_format;

    buffer =
      audio::create_buffer_memory(samples_per_second, num_channels,
                                  channel_format);

    for(auto buffer_idx = 0;
        buffer_idx != audio::TOTAL_NUM_BUFFERS;
        ++buffer_idx)
    {
      if(buffer == nullptr) {
        break;
      }

      if(audio::write_info(buffer, metadata) == false) {
        return;
      }

      this->set_duration(buffer->duration);
      this->audible_.push_back(buffer);

    } // end for TOTAL_NUM_BUFFERS loop

    this->current_buffer_ = this->audible_.begin();
  }
  NOM_DUMP(this->audible_.size());

  this->input_pos_ = 0;
}

PlayAudioSource::~PlayAudioSource()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);
}

std::unique_ptr<IActionObject> PlayAudioSource::clone() const
{
  return( nom::make_unique<self_type>( self_type(*this) ) );
}

IActionObject::FrameState
PlayAudioSource::update(real32 t, uint8 b, int16 c, real32 d)
{
  real32 delta_time = t;
  auto status = this->status();
  const real32 duration = d;
  const auto speed = this->speed();

  auto& itr = this->current_buffer_;
  if(*itr == nullptr || (*itr)->samples == nullptr) {
    status = FrameState::COMPLETED;
    this->set_status(status);
    return status;
  }

  uint32 audio_state = audio::state(*itr, this->mixer_ ? this->mixer_->engine() : nullptr);

  if(delta_time > (duration / speed)) {
    delta_time = duration / speed;
  }

  int16* samples = NOM_SCAST(int16*, (*itr)->samples);
  auto format = (*itr)->channel_format;
  auto sample_count = (*itr)->sample_count;
  auto samples_per_second = (*itr)->sample_rate;

  while(this->input_pos_ != (samples_per_second * 4.0f) &&
        this->input_pos_ != sample_count) {

    if((*itr)->samples != nullptr) {
    }

    (*itr)->samples_read =
      this->fp_->read(samples + this->input_pos_, format, samples_per_second);
    this->input_pos_ += samples_per_second;

    NOM_LOG_DEBUG(NOM_LOG_CATEGORY_TEST, "samples read:",
                 (*itr)->samples_read, "input_pos:", this->input_pos_);
  }

  if(this->mixer_ != nullptr) {
    this->mixer_->queue_buffer(*itr);
  }

  ++this->current_buffer_;
  if(this->current_buffer_ == (this->audible_.end() - 1)) {
    this->current_buffer_ = this->audible_.begin();
  }

  audio_state = audio::state((*itr), this->mixer_ ? this->mixer_->engine() : nullptr);

  if(delta_time < (duration / speed)) {
    if(audio_state != audio::AUDIO_STATE_PLAYING) {

      NOM_LOG_INFO(NOM_LOG_CATEGORY_TEST, "play!");
      if(this->mixer_ != nullptr) {
        this->mixer_->play((*this->audible_.begin()), this->bus_);
      }
    }

    status = FrameState::PLAYING;
    this->set_status(status);
  } else {
    this->last_frame(delta_time);
    status = FrameState::COMPLETED;
    this->set_status(status);
  }

  return status;
}

IActionObject::FrameState PlayAudioSource::next_frame(real32 delta_time)
{
  delta_time = this->timer_.to_seconds();
  this->first_frame(delta_time);

  return this->update(delta_time, 0.0f, 0.0f, this->duration());
}

IActionObject::FrameState PlayAudioSource::prev_frame(real32 delta_time)
{
  delta_time = this->timer_.to_seconds();
  this->first_frame(delta_time);

  return this->update(delta_time, 0.0f, 0.0f, this->duration());
}

void PlayAudioSource::pause(real32 delta_time)
{
  auto itr = this->current_buffer_;

  this->timer_.pause();
  if(this->mixer_ != nullptr) {
    this->mixer_->pause(*itr);
  }
}

void PlayAudioSource::resume(real32 delta_time)
{
  auto itr = this->current_buffer_;

  this->timer_.unpause();
  if(this->mixer_ != nullptr) {
    this->mixer_->resume(*itr);
  }
}

void PlayAudioSource::rewind(real32 delta_time)
{
  auto itr = this->current_buffer_;

  this->elapsed_frames_ = 0.0f;
  this->timer_.stop();
  this->set_status(FrameState::PLAYING);

  if((*itr) != nullptr) {
    if(this->mixer_ != nullptr) {
      this->mixer_->stop(*itr);
    }
    (*itr)->samples_read = 0;
  }
}

void PlayAudioSource::release()
{
  auto itr = this->current_buffer_;

  if(*itr != nullptr) {
    auto num_buffers = this->audible_.size();

    NOM_LOG_DEBUG(NOM_LOG_CATEGORY_TEST, "processed_buffers:", num_buffers);

    auto audible_end = this->audible_.end();
    for(auto buf_itr = this->audible_.begin();
        buf_itr != audible_end; ++buf_itr)
    {
      audio::free_buffer((*buf_itr), this->mixer_ ? this->mixer_->engine() : nullptr);
    }
    this->audible_.clear();
  }
}

// Private scope

void PlayAudioSource::first_frame(real32 delta_time)
{
  if(this->timer_.started() == false) {
    this->timer_.start();

    NOM_LOG_INFO(NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
                 "BEGIN at", delta_time);

    this->input_pos_ = 0;
  }
}

void PlayAudioSource::last_frame(real32 delta_time)
{
  NOM_LOG_INFO(NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
               "END at", delta_time);
  auto itr = this->current_buffer_;

  this->timer_.stop();
  this->input_pos_ = 0;
}

} // namespace nom
