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
#include "nomlib/actions/RemoveAction.hpp"

#include "nomlib/core/unique_ptr.hpp"

namespace nom {

// Static initializations
const char* RemoveAction::DEBUG_CLASS_NAME = "[RemoveAction]:";

RemoveAction::RemoveAction(const std::shared_ptr<IActionObject>& action)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      nom::NOM_LOG_PRIORITY_VERBOSE );

  this->action_ = action;
}

RemoveAction::~RemoveAction()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

std::unique_ptr<IActionObject> RemoveAction::clone() const
{
  auto cloned_obj = nom::make_unique<self_type>( self_type(*this) );
  if( cloned_obj != nullptr ) {

    cloned_obj->set_status(FrameState::PLAYING);
    cloned_obj->set_lifecycle_state(LifecycleState::IDLE);
    cloned_obj->elapsed_frames_ = 0.0f;
    cloned_obj->timer_.stop();

    if( this->action_ != nullptr ) {
      cloned_obj->action_ = this->action_->clone();
    }

    return std::move(cloned_obj);
  } else {
    return nullptr;
  }
}

IActionObject::FrameState RemoveAction::next_frame(real32 delta_time)
{
  if( this->lifecycle_state() == LifecycleState::RELEASED ) {
    this->set_status(FrameState::COMPLETED);
    return this->status();
  }

  this->set_lifecycle_state(LifecycleState::RUNNING);

  if( this->action_ != nullptr ) {

    std::string action_id = this->action_->name();
    if( action_id == "" ) {
      action_id = "action";
    }

    NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
                    "removing", action_id, "from container",
                    "[action_id]:", this->name() );

    this->release();
  }

  this->set_status(FrameState::COMPLETED);
  this->set_lifecycle_state(LifecycleState::FINISHED);
  return this->status();
}

IActionObject::FrameState RemoveAction::prev_frame(real32 delta_time)
{
  // NOTE: This action is not reversible
  return this->next_frame(delta_time);
}

void RemoveAction::pause(real32 delta_time)
{
  // Not supported - instantaneous action
  IActionObject::pause(delta_time);
}

void RemoveAction::resume(real32 delta_time)
{
  // Not supported - instantaneous action
  IActionObject::resume(delta_time);
}

void RemoveAction::rewind(real32 delta_time)
{
  // Not supported - instantaneous action
  IActionObject::rewind(delta_time);
}

void RemoveAction::release()
{
  if( this->action_ != nullptr ) {
    this->action_->release();
  }

  this->action_.reset();

  IActionObject::release();
}

} // namespace nom
