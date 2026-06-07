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
#include "nomlib/graphics/ViewportManager.hpp"

// Private headers (third-party)
#include <SDL.h>

// Forward declarations
#include "nomlib/graphics/Renderer.hpp"
#include "nomlib/math/math_helpers.hpp"

namespace nom {

ViewportManager::ViewportManager() :
  logical_size_( Size2i::zero ),
  output_size_( Size2i::zero ),
  viewport_( IntRect::zero ),
  scale_( Point2f( 1.0f, 1.0f ) )
{
  // NOM_LOG_TRACE( NOM );
}

ViewportManager::~ViewportManager()
{
  // NOM_LOG_TRACE( NOM );
}

void
ViewportManager::set_logical_size(const Size2i& logical_size)
{
  this->logical_size_ = logical_size;

  if( this->output_size_ != Size2i::zero ) {
    this->recalculate();
  }
}

const Size2i&
ViewportManager::logical_size() const
{
  return this->logical_size_;
}

const Size2i&
ViewportManager::output_size() const
{
  return this->output_size_;
}

const IntRect&
ViewportManager::viewport() const
{
  return this->viewport_;
}

const Point2f&
ViewportManager::scale() const
{
  return this->scale_;
}

Size2i
ViewportManager::context_size() const
{
  return this->logical_size_;
}

Point2i
ViewportManager::window_to_logical(const Point2i& window_pos) const
{
  Point2i result( Point2i::zero );

  if( this->logical_size_ == Size2i::zero || this->scale_.x == 0.0f ||
      this->scale_.y == 0.0f )
  {
    return window_pos;
  }

  result.x = NOM_SCAST( int,
    ( window_pos.x - this->viewport_.x ) / this->scale_.x );
  result.y = NOM_SCAST( int,
    ( window_pos.y - this->viewport_.y ) / this->scale_.y );

  if( result.x < 0 ) result.x = 0;
  if( result.y < 0 ) result.y = 0;
  if( result.x > this->logical_size_.w ) result.x = this->logical_size_.w;
  if( result.y > this->logical_size_.h ) result.y = this->logical_size_.h;

  return result;
}

Point2i
ViewportManager::logical_to_window(const Point2i& logical_pos) const
{
  Point2i result( Point2i::zero );

  result.x = NOM_SCAST( int,
    ( logical_pos.x * this->scale_.x ) + this->viewport_.x );
  result.y = NOM_SCAST( int,
    ( logical_pos.y * this->scale_.y ) + this->viewport_.y );

  return result;
}

void
ViewportManager::recalculate()
{
  if( this->logical_size_ == Size2i::zero ||
      this->output_size_ == Size2i::zero )
  {
    this->scale_ = Point2f( 1.0f, 1.0f );
    this->viewport_ = IntRect( 0, 0, this->output_size_.w,
                               this->output_size_.h );
    return;
  }

  real32 scale_x = NOM_SCAST( real32, this->output_size_.w ) /
                   NOM_SCAST( real32, this->logical_size_.w );
  real32 scale_y = NOM_SCAST( real32, this->output_size_.h ) /
                   NOM_SCAST( real32, this->logical_size_.h );

  real32 min_scale = nom::minimum( scale_x, scale_y );

  this->scale_.x = min_scale;
  this->scale_.y = min_scale;

  int viewport_w = NOM_SCAST( int,
    this->logical_size_.w * min_scale );
  int viewport_h = NOM_SCAST( int,
    this->logical_size_.h * min_scale );

  int viewport_x = ( this->output_size_.w - viewport_w ) / 2;
  int viewport_y = ( this->output_size_.h - viewport_h ) / 2;

  this->viewport_ = IntRect( viewport_x, viewport_y,
                             viewport_w, viewport_h );
}

void
ViewportManager::on_window_resized(const Size2i& output_size,
                                   SDL_Renderer* renderer)
{
  this->output_size_ = output_size;
  this->recalculate();

  if( renderer != nullptr ) {
    this->apply_to_renderer(renderer);
  }
}

void
ViewportManager::on_window_resized(const Size2i& output_size,
                                   Renderer* renderer)
{
  if( renderer != nullptr ) {
    this->on_window_resized(output_size, renderer->renderer());
  } else {
    this->on_window_resized(output_size, static_cast<SDL_Renderer*>(nullptr));
  }
}

void
ViewportManager::apply_to_renderer(SDL_Renderer* renderer) const
{
  if( renderer == nullptr ) {
    return;
  }

  if( this->logical_size_ != Size2i::zero ) {
    SDL_RenderSetLogicalSize( renderer, this->logical_size_.w,
                              this->logical_size_.h );
  }

  if( this->viewport_ != IntRect::zero ) {
    SDL_Rect vp = { this->viewport_.x, this->viewport_.y,
                    this->viewport_.w, this->viewport_.h };
    SDL_RenderSetViewport( renderer, &vp );
  }

  SDL_RenderSetScale( renderer, this->scale_.x, this->scale_.y );
}

void
ViewportManager::apply_to_renderer(Renderer* renderer) const
{
  if( renderer != nullptr ) {
    this->apply_to_renderer(renderer->renderer());
  }
}

} // namespace nom
