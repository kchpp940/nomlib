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
#include "nomlib/actions/CallbackAction.hpp"

#include "nomlib/core/unique_ptr.hpp"

namespace nom {

// Static initializations
const char* CallbackAction::DEBUG_CLASS_NAME = "[CallbackAction]:";

CallbackAction::CallbackAction(const callback_func& action) :
  action_(action)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      nom::NOM_LOG_PRIORITY_VERBOSE );

  this->set_duration(0.0f);
  this->elapsed_frames_ = 0.0f;
}

CallbackAction::CallbackAction(real32 seconds, const callback_func& action) :
  action_(action)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      nom::NOM_LOG_PRIORITY_VERBOSE );

  this->set_duration(seconds);
  this->elapsed_frames_ = 0.0f;
}

CallbackAction::~CallbackAction()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

std::unique_ptr<IActionObject> CallbackAction::clone() const
{
  auto cloned_obj = nom::make_unique<self_type>( self_type(*this) );
  if( cloned_obj != nullptr ) {

    cloned_obj->set_status(FrameState::PLAYING);
    cloned_obj->set_lifecycle_state(LifecycleState::IDLE);
    cloned_obj->elapsed_frames_ = 0.0f;
    cloned_obj->timer_.stop();

    return std::move(cloned_obj);
  } else {
    return nullptr;
  }
}

IActionObject::FrameState CallbackAction::next_frame(real32 delta_time)
{
  delta_time = ( Timer::to_seconds( this->timer_.ticks() ) );

  if( this->lifecycle_state() == LifecycleState::RELEASED ) {
    this->set_status(FrameState::COMPLETED);
    return this->status();
  }

  this->set_lifecycle_state(LifecycleState::RUNNING);

  if( this->timer_.started() == false ) {
    this->timer_.start();

    NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
                    "BEGIN at", delta_time );
  }

  // Clamp delta values that go beyond maximal duration
  if( delta_time > (this->duration() / this->speed() ) ) {
    delta_time = this->duration() / this->speed();
  }

  // Apply speed scalar onto current frame time
  real32 frame_time = delta_time * this->speed();

  if( this->duration() == 0.0f ) {

    ++this->elapsed_frames_;

    if( this->action_ != nullptr ) {
      this->action_.operator()();
    }

    this->set_status(FrameState::COMPLETED);
    this->set_lifecycle_state(LifecycleState::FINISHED);

  } else if( delta_time < (this->duration() / this->speed() ) ) {

    ++this->elapsed_frames_;

    if( this->action_ != nullptr ) {
      this->action_.operator()();
    }

    this->set_status(FrameState::PLAYING);

  } else {
    this->set_status(FrameState::COMPLETED);
    this->set_lifecycle_state(LifecycleState::FINISHED);
  }

  NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
                  "delta_time:", delta_time, "frame_time:", frame_time,
                  "[elapsed frames]:", this->elapsed_frames_ );

  return this->status();
}

IActionObject::FrameState CallbackAction::prev_frame(real32 delta_time)
{
  // NOTE: This action is not reversible
  return this->next_frame(delta_time);
}

void CallbackAction::pause(real32 delta_time)
{
  IActionObject::pause(delta_time);
}

void CallbackAction::resume(real32 delta_time)
{
  IActionObject::resume(delta_time);
}

void CallbackAction::rewind(real32 delta_time)
{
  IActionObject::rewind(delta_time);

  // Stop the timer so the next run through next_frame() will re-enter the
  // first_frame "begin" branch (timer_.started() == false).  This is required
  // for correct behaviour when the action is wrapped inside RepeatFor or
  // RepeatForever.
  this->timer_.stop();
}

void CallbackAction::release()
{
  this->action_ = nullptr;
  IActionObject::release();
}

} // namespace nom
