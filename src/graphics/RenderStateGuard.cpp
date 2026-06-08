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
#include "nomlib/graphics/RenderStateGuard.hpp"

// Private headers
#include "nomlib/graphics/Renderer.hpp"

#include <SDL_opengl.h>

namespace nom {

RenderStateGuard::RenderStateGuard( SDL_Renderer* renderer, Scope scope )
  : renderer_( renderer )
  , scope_( scope )
  , sdl_draw_color_( Color4i::null )
  , sdl_blend_mode_( SDL_BLENDMODE_NONE )
  , sdl_viewport_( IntRect::null )
  , sdl_clip_bounds_( IntRect::null )
  , sdl_scale_( Point2f::zero )
  , gl_scissor_enabled_( false )
  , gl_scissor_box_( IntRect::null )
  , gl_blend_enabled_( false )
  , gl_blend_src_( 0 )
  , gl_blend_dst_( 0 )
  , gl_color_{ 1.0f, 1.0f, 1.0f, 1.0f }
  , gl_vertex_array_enabled_( false )
  , gl_color_array_enabled_( false )
  , gl_tex_coord_array_enabled_( false )
  , gl_matrix_mode_( 0 )
  , gl_texture_2d_enabled_( false )
{
  NOM_ASSERT( renderer_ != nullptr );

  if( scope_ & Scope::SDLState ) {
    this->save_sdl_state();
  }
  if( scope_ & Scope::GLState ) {
    this->save_gl_state();
  }
}

RenderStateGuard::RenderStateGuard( const Renderer& renderer, Scope scope )
  : RenderStateGuard( renderer.renderer(), scope )
{
}

RenderStateGuard::~RenderStateGuard()
{
  if( scope_ & Scope::GLState ) {
    this->restore_gl_state();
  }
  if( scope_ & Scope::SDLState ) {
    this->restore_sdl_state();
  }
}

// ---------------------------------------------------------------------------
// SDL state
// ---------------------------------------------------------------------------

void RenderStateGuard::save_sdl_state()
{
  NOM_ASSERT( renderer_ != nullptr );

  Uint8 r = 0, g = 0, b = 0, a = 0;
  if( SDL_GetRenderDrawColor( renderer_, &r, &g, &b, &a ) == 0 ) {
    sdl_draw_color_ = Color4i( r, g, b, a );
  }

  SDL_BlendMode blend = SDL_BLENDMODE_NONE;
  if( SDL_GetRenderDrawBlendMode( renderer_, &blend ) == 0 ) {
    sdl_blend_mode_ = blend;
  }

  SDL_Rect vp = { 0, 0, 0, 0 };
  SDL_RenderGetViewport( renderer_, &vp );
  sdl_viewport_ = IntRect( vp.x, vp.y, vp.w, vp.h );

  SDL_Rect clip = { 0, 0, 0, 0 };
  SDL_RenderGetClipRect( renderer_, &clip );
  if( SDL_RenderIsClipEnabled( renderer_ ) == SDL_TRUE ) {
    sdl_clip_bounds_ = IntRect( clip.x, clip.y, clip.w, clip.h );
  } else {
    sdl_clip_bounds_ = IntRect::null;
  }

  float sx = 1.0f, sy = 1.0f;
  SDL_RenderGetScale( renderer_, &sx, &sy );
  sdl_scale_ = Point2f( sx, sy );
}

void RenderStateGuard::restore_sdl_state() const
{
  NOM_ASSERT( renderer_ != nullptr );

  SDL_SetRenderDrawColor( renderer_,
                          sdl_draw_color_.r, sdl_draw_color_.g,
                          sdl_draw_color_.b, sdl_draw_color_.a );

  SDL_SetRenderDrawBlendMode( renderer_, sdl_blend_mode_ );

  if( sdl_viewport_ == IntRect::null ) {
    SDL_RenderSetViewport( renderer_, nullptr );
  } else {
    SDL_Rect vp = { sdl_viewport_.x, sdl_viewport_.y,
                    sdl_viewport_.w, sdl_viewport_.h };
    SDL_RenderSetViewport( renderer_, &vp );
  }

  if( sdl_clip_bounds_ == IntRect::null ) {
    SDL_RenderSetClipRect( renderer_, nullptr );
  } else {
    SDL_Rect clip = { sdl_clip_bounds_.x, sdl_clip_bounds_.y,
                      sdl_clip_bounds_.w, sdl_clip_bounds_.h };
    SDL_RenderSetClipRect( renderer_, &clip );
  }

  SDL_RenderSetScale( renderer_, sdl_scale_.x, sdl_scale_.y );
}

// ---------------------------------------------------------------------------
// OpenGL state (for libRocket direct GL usage)
// ---------------------------------------------------------------------------

void RenderStateGuard::save_gl_state()
{
  gl_scissor_enabled_ = ( glIsEnabled( GL_SCISSOR_TEST ) == GL_TRUE );

  if( gl_scissor_enabled_ ) {
    GLint box[4] = { 0, 0, 0, 0 };
    glGetIntegerv( GL_SCISSOR_BOX, box );
    gl_scissor_box_ = IntRect( box[0], box[1], box[2], box[3] );
  }

  gl_blend_enabled_ = ( glIsEnabled( GL_BLEND ) == GL_TRUE );

  GLint blend_src = 0, blend_dst = 0;
  glGetIntegerv( GL_BLEND_SRC, &blend_src );
  glGetIntegerv( GL_BLEND_DST, &blend_dst );
  gl_blend_src_ = blend_src;
  gl_blend_dst_ = blend_dst;

  glGetFloatv( GL_CURRENT_COLOR, gl_color_ );

  gl_vertex_array_enabled_    = ( glIsEnabled( GL_VERTEX_ARRAY ) == GL_TRUE );
  gl_color_array_enabled_     = ( glIsEnabled( GL_COLOR_ARRAY ) == GL_TRUE );
  gl_tex_coord_array_enabled_ = ( glIsEnabled( GL_TEXTURE_COORD_ARRAY ) == GL_TRUE );

  GLint matrix_mode = 0;
  glGetIntegerv( GL_MATRIX_MODE, &matrix_mode );
  gl_matrix_mode_ = matrix_mode;

  gl_texture_2d_enabled_ = ( glIsEnabled( GL_TEXTURE_2D ) == GL_TRUE );
}

void RenderStateGuard::restore_gl_state() const
{
  if( gl_scissor_enabled_ ) {
    glEnable( GL_SCISSOR_TEST );
    glScissor( gl_scissor_box_.x, gl_scissor_box_.y,
               gl_scissor_box_.w, gl_scissor_box_.h );
  } else {
    glDisable( GL_SCISSOR_TEST );
  }

  if( gl_blend_enabled_ ) {
    glEnable( GL_BLEND );
  } else {
    glDisable( GL_BLEND );
  }
  glBlendFunc( gl_blend_src_, gl_blend_dst_ );

  glColor4f( gl_color_[0], gl_color_[1], gl_color_[2], gl_color_[3] );

  if( gl_vertex_array_enabled_ ) {
    glEnableClientState( GL_VERTEX_ARRAY );
  } else {
    glDisableClientState( GL_VERTEX_ARRAY );
  }

  if( gl_color_array_enabled_ ) {
    glEnableClientState( GL_COLOR_ARRAY );
  } else {
    glDisableClientState( GL_COLOR_ARRAY );
  }

  if( gl_tex_coord_array_enabled_ ) {
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
  } else {
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
  }

  glMatrixMode( gl_matrix_mode_ );

  if( gl_texture_2d_enabled_ ) {
    glEnable( GL_TEXTURE_2D );
  } else {
    glDisable( GL_TEXTURE_2D );
  }
}

} // namespace nom
