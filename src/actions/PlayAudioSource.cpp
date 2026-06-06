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
// #include "nomlib/audio/AL/ALAudioDeviceCaps.hpp"

namespace nom {

// Static initializations
const char* PlayAudioSource::DEBUG_CLASS_NAME = "[PlayAudioSource]:";

PlayAudioSource::
PlayAudioSource(audio::IOAudioEngine* dev, const char* filename)
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);

  audio::SoundBuffer* buffer = nullptr;
  this->impl_ = dev;
  this->elapsed_frames_ = 0.0f;

  this->fp_ = new audio::SoundFileReader();
  NOM_ASSERT(this->fp_ != nullptr);
  if(this->fp_ != nullptr) {

    if(this->fp_->open(filename, this->metadata_) == false) {
      this->release();
      return;
    }

    if(this->fp_->valid() == false) {
      this->release();
      return;
    }

    auto samples_per_second = this->metadata_.sample_rate;
    auto num_channels = this->metadata_.channel_count;
    auto channel_format = this->metadata_.channel_format;

    // NOTE(jeff): Create a queue of buffers to stream out in chunks
    for(auto buffer_idx = 0;
        buffer_idx != audio::TOTAL_NUM_BUFFERS;
        ++buffer_idx)
    {
      buffer =
        audio::create_buffer_memory(samples_per_second, num_channels,
                                    channel_format);

      if(buffer == nullptr) {
        this->release();
        return;
      }

      if(audio::write_info(buffer, this->metadata_) == false) {
        audio::free_buffer(buffer, this->impl_);
        this->release();
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
#if 0
PlayAudioSource::
PlayAudioSource(audio::IOAudioEngine* dev, audio::SoundBuffer* buffer)
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);

  NOM_ASSERT_INVALID_PATH("TODO");

  this->impl_ = dev;
  this->elapsed_frames_ = 0.0f;
  this->audible_ = buffer;

  if(buffer != nullptr) {
    this->set_duration(buffer->duration);
  }
}
#endif
PlayAudioSource::~PlayAudioSource()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);

  this->release();
}

std::unique_ptr<IActionObject> PlayAudioSource::clone() const
{
  return( nom::make_unique<self_type>( self_type(*this) ) );
}

IActionObject::FrameState
PlayAudioSource::update(real32 t, uint8 b, int16 c, real32 d)
{
  if(this->released_ == true) {
    auto status = FrameState::COMPLETED;
    this->set_status(status);
    return status;
  }

  real32 delta_time = t;
  auto status = this->status();
  const real32 duration = d;
  const auto speed = this->speed();

  auto& itr = this->current_buffer_;
  if(itr == this->audible_.end() || *itr == nullptr || (*itr)->samples == nullptr) {
    status = FrameState::COMPLETED;
    this->set_status(status);
    return status;
  }

  uint32 audio_state = audio::state(*itr, this->impl_);

  // Clamp delta values that go beyond the time duration bounds; this adds
  // stability to variable time steps
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
      // audio::free_samples( (*itr)->channel_format, (*itr)->samples);
    }

    (*itr)->samples_read =
      this->fp_->read(samples + this->input_pos_, format, samples_per_second);
    this->input_pos_ += samples_per_second;

    NOM_LOG_DEBUG(NOM_LOG_CATEGORY_TEST, "samples read:",
                 (*itr)->samples_read, "input_pos:", this->input_pos_);
  }

  // this->impl_->push_buffer(this->audible_);
  this->impl_->queue_buffer(*itr);

  ++this->current_buffer_;
  if(this->current_buffer_ == (this->audible_.end() - 1)) {
    this->current_buffer_ = this->audible_.begin();
  }

  audio_state = audio::state((*itr), this->impl_);

  // Continue playing the animation only when we are inside our frame duration
  // bounds; this adds stability to variable time steps
  if(delta_time < (duration / speed)) {
    if(audio_state != audio::AUDIO_STATE_PLAYING) {

      NOM_LOG_INFO(NOM_LOG_CATEGORY_TEST, "play!");
      audio::play((*this->audible_.begin()), this->impl_);
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
  if(this->released_ == true) {
    auto status = FrameState::COMPLETED;
    this->set_status(status);
    return status;
  }

  delta_time = this->timer_.to_seconds();
#if 0
  if(this->audible_) {
    this->audible_->elapsed_seconds = this->timer_.ticks();
  }
#endif
  this->first_frame(delta_time);

  return this->update(delta_time, 0.0f, 0.0f, this->duration());
}

IActionObject::FrameState PlayAudioSource::prev_frame(real32 delta_time)
{
  if(this->released_ == true) {
    auto status = FrameState::COMPLETED;
    this->set_status(status);
    return status;
  }

  delta_time = this->timer_.to_seconds();
#if 0
  if(this->audible_) {
    this->audible_->elapsed_seconds = this->timer_.ticks();
  }
#endif
  this->first_frame(delta_time);

  return this->update(delta_time, 0.0f, 0.0f, this->duration());
}

void PlayAudioSource::pause(real32 delta_time)
{
  if(this->released_ == true) {
    return;
  }

  this->timer_.pause();

  if(this->audible_.empty() == false && this->current_buffer_ != this->audible_.end()) {
    auto itr = this->current_buffer_;
    if(*itr != nullptr) {
      audio::pause((*itr), this->impl_);
    }
  }
}

void PlayAudioSource::resume(real32 delta_time)
{
  if(this->released_ == true) {
    return;
  }

  this->timer_.unpause();

  if(this->audible_.empty() == false && this->current_buffer_ != this->audible_.end()) {
    auto itr = this->current_buffer_;
    if(*itr != nullptr) {
      audio::resume((*itr), this->impl_);
    }
  }
}

void PlayAudioSource::rewind(real32 delta_time)
{
  if(this->released_ == true) {
    return;
  }

  this->elapsed_frames_ = 0.0f;
  this->timer_.stop();
  this->set_status(FrameState::PLAYING);
  this->input_pos_ = 0;

  bool queue_cleared = true;

  for(auto buffer : this->audible_) {
    if(buffer != nullptr) {
      bool ok = audio::reset_stream_queue(buffer, this->impl_);
      if(ok == false) {
        queue_cleared = false;
        audio::stop(buffer, this->impl_);
      }
      buffer->samples_read = 0;
    }
  }

  if(queue_cleared == false) {
    NOM_LOG_WARN(NOM_LOG_CATEGORY_AUDIO,
                 DEBUG_CLASS_NAME,
                 "rewind(): audio backend does not support reset_stream_queue;",
                 "falling back to stop(). Queued buffers may still be attached",
                 "to the source.");
  }

  if(this->fp_ != nullptr) {
    this->fp_->seek(0, audio::SOUND_SEEK_SET);
  }

  this->current_buffer_ = this->audible_.begin();
}

void PlayAudioSource::release()
{
  if(this->released_ == true) {
    return;
  }

  this->released_ = true;

  bool queue_cleared = true;

  for(auto buffer : this->audible_) {
    if(buffer != nullptr) {
      bool ok = audio::reset_stream_queue(buffer, this->impl_);
      if(ok == false) {
        queue_cleared = false;
        audio::stop(buffer, this->impl_);
      }
    }
  }

  if(queue_cleared == false) {
    NOM_LOG_WARN(NOM_LOG_CATEGORY_AUDIO,
                 DEBUG_CLASS_NAME,
                 "release(): audio backend does not support reset_stream_queue;",
                 "falling back to stop() before free_buffer().");
  }

  auto num_buffers = this->audible_.size();

  NOM_LOG_DEBUG(NOM_LOG_CATEGORY_TEST, "processed_buffers:", num_buffers);

  auto audible_end = this->audible_.end();
  for(auto itr = this->audible_.begin();
      itr != audible_end; ++itr)
  {
    audio::free_buffer((*itr), this->impl_);
  }

  this->audible_.clear();

  NOM_DELETE_PTR(this->fp_);
}

// Private scope

void PlayAudioSource::first_frame(real32 delta_time)
{
  if(this->released_ == true) {
    return;
  }

  if(this->timer_.started() == false) {
    this->timer_.start();

    NOM_LOG_INFO(NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
                 "BEGIN at", delta_time);

    this->input_pos_ = 0;
    // audio::play((*itr), this->impl_);
  }
}

void PlayAudioSource::last_frame(real32 delta_time)
{
  NOM_LOG_INFO(NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
               "END at", delta_time);

  this->timer_.stop();
  this->input_pos_ = 0;

  bool queue_cleared = true;

  for(auto buffer : this->audible_) {
    if(buffer != nullptr) {
      bool ok = audio::reset_stream_queue(buffer, this->impl_);
      if(ok == false) {
        queue_cleared = false;
        audio::stop(buffer, this->impl_);
      }
    }
  }

  if(queue_cleared == false) {
    NOM_LOG_WARN(NOM_LOG_CATEGORY_AUDIO,
                 DEBUG_CLASS_NAME,
                 "last_frame(): audio backend does not support reset_stream_queue;",
                 "falling back to stop().");
  }
}

} // namespace nom
