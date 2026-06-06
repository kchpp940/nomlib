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
#include "nomlib/graphics/sprite/SpriteAnimator.hpp"

// Private headers
#include "nomlib/core/unique_ptr.hpp"
#include "nomlib/ptree.hpp"

// Forward declarations
#include "nomlib/graphics/sprite/SpriteBatch.hpp"

namespace nom {

SpriteAnimator::SpriteAnimator() :
  drawable_(nullptr),
  state_(State::STOPPED),
  current_clip_(this->clips_.end()),
  current_clip_name_("\0"),
  elapsed_time_(0.0f),
  logical_frame_(0),
  direction_(1),
  completion_fired_(false)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_RENDER,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

SpriteAnimator::SpriteAnimator( SpriteBatch& drawable ) :
  drawable_(&drawable),
  state_(State::STOPPED),
  current_clip_(this->clips_.end()),
  current_clip_name_("\0"),
  elapsed_time_(0.0f),
  logical_frame_(0),
  direction_(1),
  completion_fired_(false)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_RENDER,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

SpriteAnimator::~SpriteAnimator()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_RENDER,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

SpriteAnimator* SpriteAnimator::clone() const
{
  return( new SpriteAnimator(*this) );
}

void SpriteAnimator::set_target( SpriteBatch& drawable )
{
  this->drawable_ = &drawable;
}

bool SpriteAnimator::add_clip( const SpriteAnimationClip& clip )
{
  auto pair = std::make_pair( clip.name(), clip );
  auto result = this->clips_.insert( pair );
  return result.second;
}

bool SpriteAnimator::load_clips( const Value& object )
{
  if( object.null_type() ) {
    NOM_LOG_ERR( NOM, "Could not load clips: nom::Value object was null." );
    return false;
  }

  Value fp = object;
  Value animations = fp["animations"];

  if( animations.null_type() || !animations.object_type() ) {
    NOM_LOG_ERR( NOM, "Could not load clips: 'animations' node missing or invalid." );
    return false;
  }

  bool all_ok = true;

  for( auto itr = animations.begin(); itr != animations.end(); ++itr )
  {
    Value::Iterator member(itr);
    std::string name = member.key();

    if( !itr->object_type() ) continue;

    int start = animations[name]["start"].get_int();
    int end = animations[name]["end"].get_int();
    real32 fps = animations[name]["fps"].get_float();
    bool loop = animations[name]["loop"].get_bool();
    bool ping_pong = animations[name]["ping_pong"].get_bool();

    if( fps <= 0.0f ) {
      fps = SpriteAnimationClip::DEFAULT_FPS;
    }

    SpriteAnimationClip clip( name, start, end, fps );
    clip.set_loop( loop );
    clip.set_ping_pong( ping_pong );

    if( this->add_clip( clip ) == false ) {
      NOM_LOG_DEBUG( NOM_LOG_CATEGORY_APPLICATION,
                     "SpriteAnimator::load_clips: duplicate clip skipped:",
                     name );
      all_ok = false;
    }
  }

  return all_ok;
}

bool SpriteAnimator::remove_clip( const std::string& name )
{
  auto itr = this->clips_.find( name );
  if( itr == this->clips_.end() ) return false;

  if( this->current_clip_ == itr ) {
    this->stop();
  }

  this->clips_.erase( itr );
  return true;
}

void SpriteAnimator::remove_clips()
{
  this->stop();
  this->clips_.clear();
  this->current_clip_ = this->clips_.end();
  this->current_clip_name_.clear();
}

nom::size_type SpriteAnimator::num_clips() const
{
  return this->clips_.size();
}

const SpriteAnimationClip* SpriteAnimator::clip( const std::string& name ) const
{
  auto itr = this->clips_.find( name );
  if( itr == this->clips_.end() ) return nullptr;
  return &( itr->second );
}

bool SpriteAnimator::play( const std::string& name )
{
  auto itr = this->clips_.find( name );
  if( itr == this->clips_.end() ) {
    NOM_LOG_ERR( NOM_LOG_CATEGORY_APPLICATION,
                 "SpriteAnimator::play: unknown clip:", name );
    return false;
  }

  this->current_clip_ = itr;
  this->current_clip_name_ = name;
  this->reset_playback_state();

  this->state_ = State::PLAYING;
  this->timer_.start();

  if( this->drawable_ != nullptr ) {
    this->drawable_->set_frame( this->compute_sheet_frame() );
  }

  return true;
}

void SpriteAnimator::pause()
{
  if( this->state_ == State::PLAYING ) {
    this->state_ = State::PAUSED;
    this->timer_.pause();
  }
}

void SpriteAnimator::resume()
{
  if( this->state_ == State::PAUSED ) {
    this->state_ = State::PLAYING;
    this->timer_.unpause();
  }
}

void SpriteAnimator::stop()
{
  this->state_ = State::STOPPED;
  this->timer_.stop();
  this->reset_playback_state();
}

SpriteAnimator::State SpriteAnimator::update( real32 delta_time )
{
  if( this->state_ != State::PLAYING ) {
    return this->state_;
  }

  if( this->current_clip_ == this->clips_.end() ) {
    return this->state_;
  }

  const SpriteAnimationClip& clip = this->current_clip_->second;

  if( clip.fps() <= 0.0f ) {
    return this->state_;
  }

  real32 frame_duration = 1.0f / clip.fps();
  this->elapsed_time_ += delta_time;

  int total_logical = clip.num_frames();
  if( total_logical <= 0 ) total_logical = 1;

  while( this->elapsed_time_ >= frame_duration )
  {
    this->elapsed_time_ -= frame_duration;
    this->logical_frame_ += this->direction_;

    bool at_end = ( this->direction_ > 0 &&
                    this->logical_frame_ >= total_logical );
    bool at_start = ( this->direction_ < 0 &&
                      this->logical_frame_ <= 0 );

    if( at_end || at_start )
    {
      if( clip.ping_pong() && total_logical > 1 )
      {
        if( at_end ) {
          this->logical_frame_ = total_logical - 1;
          this->direction_ = -1;
        } else {
          this->logical_frame_ = 0;
          this->direction_ = 1;

          if( !clip.loop() ) {
            this->logical_frame_ = 0;
            this->fire_completion_if_needed();
            this->state_ = State::STOPPED;
            this->timer_.stop();
            break;
          }
        }
      }
      else if( clip.loop() )
      {
        this->logical_frame_ = 0;
        this->direction_ = 1;
      }
      else
      {
        if( at_end ) {
          this->logical_frame_ = total_logical - 1;
        } else {
          this->logical_frame_ = 0;
        }
        this->fire_completion_if_needed();
        this->state_ = State::STOPPED;
        this->timer_.stop();
        break;
      }
    }
  }

  if( this->drawable_ != nullptr && this->state_ != State::STOPPED ) {
    this->drawable_->set_frame( this->compute_sheet_frame() );
  }

  return this->state_;
}

SpriteAnimator::State SpriteAnimator::state() const
{
  return this->state_;
}

const std::string& SpriteAnimator::current_clip_name() const
{
  return this->current_clip_name_;
}

int SpriteAnimator::current_frame() const
{
  return this->compute_sheet_frame();
}

bool SpriteAnimator::playing() const
{
  return this->state_ == State::PLAYING;
}

bool SpriteAnimator::paused() const
{
  return this->state_ == State::PAUSED;
}

bool SpriteAnimator::stopped() const
{
  return this->state_ == State::STOPPED;
}

// -- Private scope ---------------------------------------------------------

void SpriteAnimator::reset_playback_state()
{
  this->elapsed_time_ = 0.0f;
  this->logical_frame_ = 0;
  this->direction_ = 1;
  this->completion_fired_ = false;
}

int SpriteAnimator::compute_sheet_frame() const
{
  if( this->current_clip_ == this->clips_.end() ) return 0;

  const SpriteAnimationClip& clip = this->current_clip_->second;
  int start = clip.start_frame();
  int end = clip.end_frame();
  int span = std::abs( end - start ) + 1;
  int ascending = ( end >= start ) ? 1 : -1;

  int logical = this->logical_frame_;

  if( clip.ping_pong() && span > 1 )
  {
    int total_logical = clip.num_frames();
    if( logical < 0 ) logical = 0;
    if( logical >= total_logical ) logical = total_logical - 1;

    int forward_span = span;
    if( logical < forward_span ) {
      return start + ascending * logical;
    } else {
      int backward = logical - ( forward_span - 1 );
      return start + ascending * ( ( forward_span - 1 ) - backward );
    }
  }

  if( span <= 0 ) return start;
  logical = logical % span;
  if( logical < 0 ) logical += span;

  return start + ascending * logical;
}

void SpriteAnimator::fire_completion_if_needed()
{
  if( this->completion_fired_ ) return;
  this->completion_fired_ = true;

  if( this->current_clip_ == this->clips_.end() ) return;

  const SpriteAnimationClip::completion_callback& cb =
    this->current_clip_->second.on_complete();

  if( cb != nullptr ) {
    cb.operator()();
  }
}

} // namespace nom
