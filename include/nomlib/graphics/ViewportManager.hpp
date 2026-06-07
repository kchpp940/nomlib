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
#ifndef NOMLIB_GRAPHICS_VIEWPORT_MANAGER_HPP
#define NOMLIB_GRAPHICS_VIEWPORT_MANAGER_HPP

#include <functional>

#include "nomlib/config.hpp"
#include "nomlib/math/Rect.hpp"
#include "nomlib/math/Point2.hpp"
#include "nomlib/math/Size2.hpp"

// Forward declarations (third-party)
struct SDL_Renderer;

namespace nom {

// Forward declarations
class Renderer;

/// \brief Unified manager for logical resolution, letterbox viewport,
///        render scale, mouse coordinate inverse calculation, and
///        libRocket context dimensions.
///
/// This class consolidates viewport state that was previously handled
/// independently by RenderWindow, UIContext, and the event system.
/// When the window is resized, call ::on_window_resized and then query
/// the updated state through the accessor methods -- graphics rendering,
/// GUI input, and system event coordinates all read from the same
/// source of truth.
class ViewportManager
{
  public:
    typedef ViewportManager self_type;

    /// \brief Callback invoked whenever the viewport state changes.
    ///
    /// Fired after ::recalculate completes -- logical size, output size,
    /// viewport rect, or scale factor have all been updated.
    typedef std::function<void(const self_type&)> OnChangeCallback;

    ViewportManager();

    ~ViewportManager();

    /// \brief Set the fixed virtual (logical) resolution.
    ///
    /// This is the resolution your game logic works in. The viewport
    /// will be calculated to maintain this aspect ratio.
    ///
    /// If an output size and renderer are already bound, the new state
    /// is applied to the renderer and the change callback fires.
    void set_logical_size(const Size2i& logical_size);

    /// \brief Get the fixed virtual (logical) resolution.
    const Size2i& logical_size() const;

    /// \brief Get the actual output (window) size in pixels.
    const Size2i& output_size() const;

    /// \brief Get the letterbox viewport rectangle in window pixel coordinates.
    const IntRect& viewport() const;

    /// \brief Get the render scale factor (output / logical).
    const Point2f& scale() const;

    /// \brief Get the dimensions to use for libRocket context.
    ///
    /// This equals the logical size when letterboxing, since libRocket
    /// works in logical coordinates.
    Size2i context_size() const;

    /// \brief Convert a window-pixel mouse coordinate to logical coordinates.
    ///
    /// Handles the letterbox offset and inverse scaling. If the input
    /// coordinate is outside the letterbox area, it is clamped to the
    /// nearest edge.
    Point2i window_to_logical(const Point2i& window_pos) const;

    /// \brief Convert a logical coordinate to window-pixel coordinates.
    Point2i logical_to_window(const Point2i& logical_pos) const;

    /// \brief Bind a renderer for automatic state application.
    ///
    /// When set, changes to logical size or window resize automatically
    /// apply the new viewport and scale to this renderer via
    /// ::apply_to_renderer. Pass nullptr to unbind.
    void set_renderer(Renderer* renderer);

    /// \brief Register a callback for viewport state changes.
    ///
    /// The callback is fired after any recalculation completes -- both
    /// from ::set_logical_size (when output_size is already known) and
    /// from ::on_window_resized.
    ///
    /// Typical use: UIContext registers here to call SetDimensions on
    /// the libRocket context whenever the logical size changes.
    void set_on_change_callback(OnChangeCallback cb);

    /// \brief Notify the manager that the output window has been resized.
    ///
    /// Recalculates the letterbox viewport and scale factor, applies
    /// the new state to the bound renderer (if any), and fires the
    /// change callback.
    ///
    /// \param output_size The new window size in pixels.
    /// \param renderer    Optional pointer to SDL_Renderer to apply state
    ///                    to; overrides the bound renderer for this call.
    void on_window_resized(const Size2i& output_size, SDL_Renderer* renderer = nullptr);

    /// \brief Notify the manager that the output window has been resized.
    ///
    /// Overload that takes a nom::Renderer.
    void on_window_resized(const Size2i& output_size, Renderer* renderer);

    /// \brief Apply the current viewport and scale to the SDL renderer.
    ///
    /// This is called automatically by ::on_window_resized and by
    /// ::set_logical_size (when a renderer is bound), but may be
    /// invoked manually if the rendering context has been changed.
    void apply_to_renderer(SDL_Renderer* renderer) const;

    /// \brief Apply the current viewport and scale to the nomlib renderer.
    void apply_to_renderer(Renderer* renderer) const;

  private:
    /// \brief Recalculate viewport and scale from stored sizes,
    ///        apply to bound renderer if present, and fire the change callback.
    void recalculate();

    /// \brief Fixed virtual resolution.
    Size2i logical_size_;

    /// \brief Actual output window size in pixels.
    Size2i output_size_;

    /// \brief Letterbox viewport in output (window) pixel coordinates.
    IntRect viewport_;

    /// \brief Rendering scale factor applied to the SDL renderer.
    Point2f scale_;

    /// \brief Non-owning pointer to the renderer for automatic state application.
    Renderer* renderer_ = nullptr;

    /// \brief Optional callback fired after every recalculation.
    OnChangeCallback on_change_;
};

} // namespace nom

#endif // include guard defined
