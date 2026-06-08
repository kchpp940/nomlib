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
#ifndef NOMLIB_GRAPHICS_RENDER_STATE_GUARD_HPP
#define NOMLIB_GRAPHICS_RENDER_STATE_GUARD_HPP

#include "nomlib/config.hpp"
#include "nomlib/math/Color4.hpp"
#include "nomlib/math/Rect.hpp"
#include "nomlib/math/Point2.hpp"

#include <SDL.h>

namespace nom {

// Forward declarations
class Renderer;

/// \brief RAII guard for saving and restoring SDL + OpenGL rendering state.
///
/// Use this whenever you need to make raw SDL/GL state changes (especially
/// in GUI rendering code) and must guarantee that the original rendering
/// state is restored upon exit -- even if an exception is thrown.
///
/// \note This guard intentionally captures **both** the high-level SDL
///       Renderer state AND the low-level OpenGL state that libRocket's
///       rendering interface bypasses SDL to touch directly.
///
/// Usage:
/// \code
///   {
///     RenderStateGuard guard(renderer);
///     // ... make raw GL or SDL state changes here ...
///   } // state is automatically restored here
/// \endcode
class RenderStateGuard
{
  public:
    enum class Scope : uint32
    {
      SDLState       = 1 << 0,
      GLState        = 1 << 1,
      All            = SDLState | GLState
    };

    /// \brief Save rendering state from the given SDL_Renderer.
    ///
    /// \param renderer   Raw SDL renderer pointer (must not be null).
    /// \param scope      Which parts of state to save/restore.
    explicit RenderStateGuard( SDL_Renderer* renderer,
                               Scope scope = Scope::All );

    /// \brief Save rendering state from a Renderer object.
    explicit RenderStateGuard( const Renderer& renderer,
                               Scope scope = Scope::All );

    /// \brief Restore all saved rendering state.
    ~RenderStateGuard();

    RenderStateGuard( const RenderStateGuard& ) = delete;
    RenderStateGuard& operator=( const RenderStateGuard& ) = delete;

  private:
    void save_sdl_state();
    void restore_sdl_state() const;

    void save_gl_state();
    void restore_gl_state() const;

    SDL_Renderer*  renderer_;
    Scope          scope_;

    // -------------------------------------------------------
    // SDL renderer state snapshot
    // -------------------------------------------------------
    Color4i        sdl_draw_color_;
    SDL_BlendMode  sdl_blend_mode_;
    IntRect        sdl_viewport_;
    IntRect        sdl_clip_bounds_;
    Point2f        sdl_scale_;

    // -------------------------------------------------------
    // OpenGL state snapshot (for libRocket GL bypass code)
    // -------------------------------------------------------
    bool           gl_scissor_enabled_;
    IntRect        gl_scissor_box_;

    bool           gl_blend_enabled_;
    int            gl_blend_src_;
    int            gl_blend_dst_;

    float          gl_color_[4];

    bool           gl_vertex_array_enabled_;
    bool           gl_color_array_enabled_;
    bool           gl_tex_coord_array_enabled_;

    int            gl_matrix_mode_;
    bool           gl_texture_2d_enabled_;
};

inline RenderStateGuard::Scope
operator|( RenderStateGuard::Scope a, RenderStateGuard::Scope b )
{
  return static_cast<RenderStateGuard::Scope>(
    static_cast<uint32>(a) | static_cast<uint32>(b) );
}

inline bool
operator&( RenderStateGuard::Scope a, RenderStateGuard::Scope b )
{
  return ( static_cast<uint32>(a) & static_cast<uint32>(b) ) != 0;
}

} // namespace nom

#endif // include guard defined
