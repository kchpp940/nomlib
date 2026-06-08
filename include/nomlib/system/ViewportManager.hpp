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
#ifndef NOMLIB_SYSTEM_VIEWPORT_MANAGER_HPP
#define NOMLIB_SYSTEM_VIEWPORT_MANAGER_HPP

#include "nomlib/config.hpp"
#include "nomlib/math/Point2.hpp"
#include "nomlib/math/Rect.hpp"

namespace nom {

/// \brief Manages the logical viewport state for independent resolution
///        scaling.
///
/// Encapsulates the two components of SDL2's logical viewport transform:
///   1. **Letterbox offset** — the (x, y) position of the rendering area
///      within the physical window (from SDL_RenderGetViewport).
///   2. **Render scale** — the (scale_x, scale_y) factors between logical
///      and physical pixel units (from SDL_RenderGetScale).
///
/// The full window→logical coordinate transform is:
///   logical_x = (window_x - viewport.x) / scale_x
///   logical_y = (window_y - viewport.y) / scale_y
///
/// This structure is intentionally module-agnostic (stores plain math types)
/// so that both the system module (EventHandler) and the graphics module
/// (Renderer) can share the same state without a dependency cycle. The
/// graphics module is responsible for updating this structure whenever the
/// Renderer's logical size, scale, or viewport changes.
struct ViewportManager
{
  /// \brief Letterbox viewport bounds. x, y are the offset within the
  ///        physical window; w, h are the size of the rendering area in
  ///        physical pixels.
  IntRect viewport = IntRect(0, 0, 0, 0);

  /// \brief Render scale factors (logical_size = physical_size / scale).
  Point2f scale = Point2f(1.0f, 1.0f);

  /// \brief Whether a viewport has been bound. When false, coordinates
  ///        are passed through unchanged.
  bool bound = false;

  /// \brief Convert a raw window-space mouse position to logical space.
  Point2i to_logical(const Point2i& window_pos) const;

  /// \brief Convert a raw window-space mouse delta to logical space.
  Point2i to_logical_delta(const Point2i& window_delta) const;
};

} // namespace nom

#endif // include guard defined
