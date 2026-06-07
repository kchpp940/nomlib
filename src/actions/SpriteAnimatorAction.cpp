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
#include "nomlib/actions/SpriteAnimatorAction.hpp"

// Private headers
#include "nomlib/core/unique_ptr.hpp"

// Forward declarations
#include "nomlib/graphics/sprite/Sprite.hpp"
#include "nomlib/graphics/sprite/SpriteAnimationClip.hpp"
#include "nomlib/graphics/sprite/SpriteAnimator.hpp"

namespace nom {

// Static initializations
const char* SpriteAnimatorAction::DEBUG_CLASS_NAME = "[SpriteAnimatorAction]:";

SpriteAnimatorAction::SpriteAnimatorAction(
  const std::shared_ptr<Sprite>& drawable,
  const std::string& clip_name ) :
  drawable_(drawable),
  clip_name_(clip_name),
  started_(false)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      nom::NOM_LOG_PRIORITY_VERBOSE );

  this->elapsed_frames_ = 0.0f;

  // Derive an initial duration estimate from the clip (if registered already).
  if( this->drawable_ != nullptr ) {
    const SpriteAnimationClip* clip =
      this->drawable_->animator().clip( clip_name );
    if( clip != nullptr ) {
      this->set_duration( clip->duration() );
    }
  }
}

SpriteAnimatorAction::~SpriteAnimatorAction()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_ACTION,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

std::unique_ptr<IActionObject> SpriteAnimatorAction::clone() const
{
  return( nom::make_unique<self_type>( self_type(*this) ) );
}

IActionObject::FrameState
SpriteAnimatorAction::next_frame( real32 delta_time )
{
  if( this->drawable_ == nullptr ) {
    this->set_status( FrameState::COMPLETED );
    return this->status();
  }

  if( !this->started_ ) {
    if( this->drawable_->play_animation( this->clip_name_ ) == false ) {
      NOM_LOG_ERR( NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
                   "Failed to start clip:", this->clip_name_ );
      this->set_status( FrameState::COMPLETED );
      return this->status();
    }
    this->started_ = true;
    this->timer_.start();
  }

  real32 elapsed = Timer::to_seconds( this->timer_.ticks() );
  real32 speed = this->speed();
  if( speed <= 0.0f ) speed = 1.0f;

  real32 effective_delta = delta_time * speed;

  SpriteAnimator::State anim_state =
    this->drawable_->update_animation( effective_delta );

  ++this->elapsed_frames_;

  NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION, DEBUG_CLASS_NAME,
                  "clip:", this->clip_name_,
                  "elapsed:", elapsed,
                  "elapsed_frames:", this->elapsed_frames_ );

  // If the clip is non-looping and the animator reports STOPPED, the action
  // is done.
  const SpriteAnimationClip* clip =
    this->drawable_->animator().clip( this->clip_name_ );
  bool non_looping_done = ( anim_state == SpriteAnimator::State::STOPPED &&
                            clip != nullptr && !clip->loop() );

  // Also check if we've blown past the configured duration as a safety net.
  bool past_duration = ( this->duration() > 0.0f &&
                         elapsed >= ( this->duration() / speed ) );

  if( non_looping_done || past_duration ) {
    this->set_status( FrameState::COMPLETED );
  } else {
    this->set_status( FrameState::PLAYING );
  }

  return this->status();
}

IActionObject::FrameState
SpriteAnimatorAction::prev_frame( real32 delta_time )
{
  // NOTE: SpriteAnimator itself does not support scrubbing backwards. We treat
  // a reversed action as simply re-playing the clip from the start, which
  // mirrors the behaviour of other irreversible actions in the engine.
  return this->next_frame( delta_time );
}

void SpriteAnimatorAction::pause( real32 delta_time )
{
  this->timer_.pause();
  if( this->drawable_ != nullptr ) {
    this->drawable_->pause_animation();
  }
}

void SpriteAnimatorAction::resume( real32 delta_time )
{
  this->timer_.unpause();
  if( this->drawable_ != nullptr ) {
    this->drawable_->resume_animation();
  }
}

void SpriteAnimatorAction::rewind( real32 delta_time )
{
  this->elapsed_frames_ = 0.0f;
  this->started_ = false;
  this->timer_.stop();

  if( this->drawable_ != nullptr ) {
    this->drawable_->stop_animation();
  }

  this->set_status( FrameState::PLAYING );
}

void SpriteAnimatorAction::release()
{
  if( this->drawable_ != nullptr ) {
    this->drawable_->release_texture();
  }
  this->drawable_.reset();
}

} // namespace nom
