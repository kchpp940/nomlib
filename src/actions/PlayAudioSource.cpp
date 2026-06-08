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
#include "nomlib/audio/SoundFile.hpp"

namespace nom {

// Static initializations
const char* PlayAudioSource::DEBUG_CLASS_NAME = "[PlayAudioSource]:";

PlayAudioSource::
PlayAudioSource(audio::IOAudioEngine* dev, const char* filename) :
  source_filename_( filename != nullptr ? filename : "")
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);

  this->impl_ = dev;
  this->elapsed_frames_ = 0.0f;
  this->input_pos_ = 0;
  this->curr_frame_ = 0;

  this->open_source();
}

PlayAudioSource::~PlayAudioSource()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION,
                     nom::NOM_LOG_PRIORITY_VERBOSE);

  // Safety net for objects that are destroyed outside the ActionPlayer
  // lifecycle (e.g. a shared_ptr dropped early, or a stack-allocated action, or a
  // container action destructed without going through final_release()).  The
  // derived vtable is still fully intact here, so the virtual release() hook
  // dispatches correctly to PlayAudioSource::release() and the owned
  // SoundBuffer* queue / ISoundFileReader are actually freed.
  //
  // final_release() itself short-circuits when already RELEASED, so calling it here is
  // harmless even if ActionPlayer already tore the action down.
  this->final_release();
}

bool PlayAudioSource::open_source()
{
  using namespace audio;

  if( this->impl_ == nullptr || this->source_filename_.empty() ) {
    return false;
  }

  SoundBuffer* buffer = nullptr;
  SoundInfo metadata = {};

  this->fp_ = nom::make_unique<SoundFileReader>();
  if( this->fp_ == nullptr ) {
    return false;
  }

  if( this->fp_->open(this->source_filename_, metadata) == false) {
    this->fp_.reset();
    return false;
  }

  if( this->fp_->valid() == false) {
    this->fp_.reset();
    return false;
  }

  auto samples_per_second = metadata.sample_rate;
  auto num_channels = metadata.channel_count;
  auto channel_format = metadata.channel_format;

  buffer =
    create_buffer_memory(samples_per_second, num_channels,
                        channel_format);

  // NOTE(jeff): Create a queue of buffers to stream out in chunks
  for(auto buffer_idx = 0;
      buffer_idx != TOTAL_NUM_BUFFERS;
      ++buffer_idx)
  {
    if(buffer == nullptr) {
      break;
    }

    // TODO(jeff): Validity check..?
    if(write_info(buffer, metadata) == false) {
      return false;
    }

    this->set_duration(buffer->duration);
    this->audible_.push_back(buffer);

  } // end for TOTAL_NUM_BUFFERS loop

  this->current_buffer_ = this->audible_.begin();

  NOM_DUMP(this->audible_.size());

  this->input_pos_ = 0;

  return true;
}

std::unique_ptr<IActionObject> PlayAudioSource::clone() const
{
  // NOTE: We cannot use the copy constructor because unique_ptr<ISoundFileReader>
  // makes PlayAudioSource non-copyable.  Construct a fresh instance instead and
  // copy over the configuration parameters (name, duration, speed, timing curve).
  auto cloned_obj =
    nom::make_unique<self_type>(this->impl_, this->source_filename_.c_str());
  if( cloned_obj != nullptr ) {

    cloned_obj->set_name(this->name());
    cloned_obj->set_duration(this->duration());
    cloned_obj->set_speed(this->speed());
    cloned_obj->set_timing_curve(this->timing_curve());

    cloned_obj->set_status(FrameState::PLAYING);
    cloned_obj->set_lifecycle_state(LifecycleState::IDLE);
    cloned_obj->elapsed_frames_ = 0.0f;
    cloned_obj->timer_.stop();
    cloned_obj->curr_frame_ = 0;
    cloned_obj->input_pos_ = 0;

    // open_source() was already called by the constructor above.
    return std::move(cloned_obj);
  } else {
    return nullptr;
  }
}

IActionObject::FrameState
PlayAudioSource::update(real32 t, uint8 b, int16 c, real32 d)
{
  real32 delta_time = t;
  auto status = this->status();
  const real32 duration = d;
  const auto speed = this->speed();

  if( this->lifecycle_state() == LifecycleState::RELEASED ) {
    this->set_status(FrameState::COMPLETED);
    return this->status();
  }

  this->set_lifecycle_state(LifecycleState::RUNNING);

  auto& itr = this->current_buffer_;
  if( this->audible_.empty() ) {
    status = FrameState::COMPLETED;
    this->set_status(status);
    this->set_lifecycle_state(LifecycleState::FINISHED);
    return status;
  }

  if( itr == this->audible_.end() || *itr == nullptr || (*itr)->samples == nullptr) {
    status = FrameState::COMPLETED;
    this->set_status(status);
    this->set_lifecycle_state(LifecycleState::FINISHED);
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
    this->set_lifecycle_state(LifecycleState::FINISHED);
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
  IActionObject::pause(delta_time);

  auto itr = this->current_buffer_;
  if(itr != this->audible_.end() && *itr != nullptr) {
    audio::pause((*itr), this->impl_);
  }
}

void PlayAudioSource::resume(real32 delta_time)
{
  IActionObject::resume(delta_time);

  auto itr = this->current_buffer_;
  if(itr != this->audible_.end() && *itr != nullptr) {
    audio::resume((*itr), this->impl_);
  }
}

void PlayAudioSource::rewind(real32 delta_time)
{
  IActionObject::rewind(delta_time);

  this->curr_frame_ = 0;
  this->input_pos_ = 0;
  this->current_buffer_ = this->audible_.begin();

  // Stop all buffers
  for(auto itr = this->audible_.begin(); itr != this->audible_.end(); ++itr) {
    if((*itr) != nullptr) {
      audio::stop((*itr), this->impl_);
      (*itr)->samples_read = 0;
    }
  }

  // Seek the file reader back to frame zero so the next read starts from the
  // beginning of the audio.
  if( this->fp_ != nullptr && this->fp_->valid() ) {
    this->fp_->seek(0, audio::SOUND_SEEK_SET);
  }
}

void PlayAudioSource::release()
{
  // Release owned streaming buffer queue
  auto audible_end = this->audible_.end();
  for(auto itr = this->audible_.begin(); itr != audible_end; ++itr)
  {
    if(*itr != nullptr) {
      audio::free_buffer((*itr), this->impl_);
    }
  }
  this->audible_.clear();
  this->current_buffer_ = this->audible_.end();

  // unique_ptr automatically cleans up the ISoundFileReader on reset
  this->fp_.reset();

  // Observer pointer - clear, never delete
  this->impl_ = nullptr;

  IActionObject::release();
}

// Private scope

void PlayAudioSource::first_frame(real32 delta_time)
{
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

  // TODO(jeff): ?
  // audio::stop((*this->current_buffer_), this->impl_);
  // (*this->current_buffer_)->samples_read = 0;
  this->input_pos_ = 0;
}

} // namespace nom
