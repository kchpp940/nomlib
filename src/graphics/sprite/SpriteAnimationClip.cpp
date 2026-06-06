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
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
#include "nomlib/graphics/sprite/SpriteAnimationClip.hpp"

#include <algorithm>
#include <cmath>

namespace nom {

const real32 SpriteAnimationClip::DEFAULT_FPS = 15.0f;

SpriteAnimationClip::SpriteAnimationClip() :
  name_("\0"),
  start_frame_(0),
  end_frame_(0),
  fps_(DEFAULT_FPS),
  loop_(false),
  ping_pong_(false),
  on_complete_(nullptr)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_RENDER,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

SpriteAnimationClip::SpriteAnimationClip( const std::string& name,
                                      int start_frame,
                                      int end_frame,
                                      real32 fps ) :
  name_(name),
  start_frame_(start_frame),
  end_frame_(end_frame),
  fps_(fps),
  loop_(false),
  ping_pong_(false),
  on_complete_(nullptr)
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_RENDER,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

SpriteAnimationClip::~SpriteAnimationClip()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_RENDER,
                      nom::NOM_LOG_PRIORITY_VERBOSE );
}

SpriteAnimationClip* SpriteAnimationClip::clone() const
{
  return( new SpriteAnimationClip(*this) );
}

const std::string& SpriteAnimationClip::name() const
{
  return this->name_;
}

int SpriteAnimationClip::start_frame() const
{
  return this->start_frame_;
}

int SpriteAnimationClip::end_frame() const
{
  return this->end_frame_;
}

real32 SpriteAnimationClip::fps() const
{
  return this->fps_;
}

bool SpriteAnimationClip::loop() const
{
  return this->loop_;
}

bool SpriteAnimationClip::ping_pong() const
{
  return this->ping_pong_;
}

const SpriteAnimationClip::completion_callback&
SpriteAnimationClip::on_complete() const
{
  return this->on_complete_;
}

int SpriteAnimationClip::num_frames() const
{
  int span = std::abs( this->end_frame_ - this->start_frame_ ) + 1;
  if( this->ping_pong_ && span > 1 ) {
    return ( span * 2 ) - 2;
  }
  return span;
}

real32 SpriteAnimationClip::duration() const
{
  if( this->fps_ <= 0.0f ) return 0.0f;
  return static_cast<real32>( this->num_frames() ) / this->fps_;
}

void SpriteAnimationClip::set_name( const std::string& name )
{
  this->name_ = name;
}

void SpriteAnimationClip::set_start_frame( int frame )
{
  this->start_frame_ = frame;
}

void SpriteAnimationClip::set_end_frame( int frame )
{
  this->end_frame_ = frame;
}

void SpriteAnimationClip::set_frame_range( int start, int end )
{
  this->start_frame_ = start;
  this->end_frame_ = end;
}

void SpriteAnimationClip::set_fps( real32 fps )
{
  this->fps_ = fps;
}

void SpriteAnimationClip::set_loop( bool enabled )
{
  this->loop_ = enabled;
}

void SpriteAnimationClip::set_ping_pong( bool enabled )
{
  this->ping_pong_ = enabled;
}

void SpriteAnimationClip::set_completion_callback(
  const completion_callback& cb )
{
  this->on_complete_ = cb;
}

} // namespace nom
