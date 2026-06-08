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
#include "nomlib/actions/IActionObject.hpp"

// Private headers
#include "nomlib/actions/ActionTimingCurves.hpp"

namespace nom {

namespace {
// Global monotonically increasing counter used to assign every IActionObject
// a unique identity at construction time.  We deliberately use a process-wide
// counter so that ids are stable even across multiple ActionPlayer instances.
static uint64 next_action_id_counter = 0;
} // namespace (anonymous)

IActionObject::IActionObject() :
  timing_curve_(nom::Linear::ease_in_out)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      NOM_LOG_PRIORITY_VERBOSE );

  this->action_id_ = ++(next_action_id_counter);
}

IActionObject::~IActionObject()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      NOM_LOG_PRIORITY_VERBOSE );

  // NOTE: We deliberately do NOT call release() / final_release() here.  In
  // a C++ base-class destructor the derived vtable has already been torn
  // down, so invoking the virtual release() hook would only run
  // IActionObject::release() and silently skip every derived override --
  // leaking unique_ptr members, raw SoundBuffer pointers, and any other
  // resource owned by a subclass.
  //
  // The correct release path is:
  //   ActionPlayer (removes action from map)
  //     -> action->final_release()  [non-virtual, dispatches to virtual release()]
  //       -> ~IActionObject()       [RAII members destroyed bottom-up]
}

void IActionObject::final_release()
{
  if( this->lifecycle_state_ == LifecycleState::RELEASED ) {
    return;
  }

  // Dispatch to the virtual release() hook.  Because final_release() is
  // called while the object is still fully alive (typically by ActionPlayer
  // just before erasing it from its map), the vtable is intact and the
  // correct derived override runs.
  this->release();

  // Guard against a derived override that forgot to chain up.
  this->lifecycle_state_ = LifecycleState::RELEASED;
}

uint64 IActionObject::id() const
{
  return this->action_id_;
}

const std::string& IActionObject::name() const
{
  return this->name_;
}

real32 IActionObject::duration() const
{
  return this->duration_;
}

real32 IActionObject::speed() const
{
  return this->speed_;
}

const
IActionObject::timing_curve_func& IActionObject::timing_curve() const
{
  return this->timing_curve_;
}

IActionObject::LifecycleState IActionObject::lifecycle_state() const
{
  return this->lifecycle_state_;
}

void IActionObject::set_name(const std::string& action_id)
{
  this->name_ = action_id;
}

void IActionObject::set_speed(real32 speed)
{
  // Default implementation
  this->speed_ = speed;
}

void
IActionObject::set_timing_curve(const IActionObject::timing_curve_func& mode)
{
  // Default implementation
  this->timing_curve_ = mode;
}

void IActionObject::pause(real32 delta_time)
{
  (void)delta_time;
  if( this->lifecycle_state_ == LifecycleState::RUNNING ) {
    this->timer_.pause();
    this->lifecycle_state_ = LifecycleState::PAUSED;
  }
}

void IActionObject::resume(real32 delta_time)
{
  (void)delta_time;
  if( this->lifecycle_state_ == LifecycleState::PAUSED ) {
    this->timer_.unpause();
    this->lifecycle_state_ = LifecycleState::RUNNING;
  }
}

void IActionObject::rewind(real32 delta_time)
{
  (void)delta_time;
  this->elapsed_frames_ = 0.0f;
  this->timer_.stop();
  this->status_ = FrameState::PLAYING;
  this->lifecycle_state_ = LifecycleState::IDLE;
}

void IActionObject::release()
{
  this->lifecycle_state_ = LifecycleState::RELEASED;
}

// Protected scope

IActionObject::FrameState IActionObject::status() const
{
  return this->status_;
}

void IActionObject::set_duration(real32 seconds)
{
  this->duration_ = seconds;
}

void IActionObject::set_status(FrameState state)
{
  this->status_ = state;
}

void IActionObject::set_lifecycle_state(LifecycleState state)
{
  this->lifecycle_state_ = state;
}

} // namespace nom
