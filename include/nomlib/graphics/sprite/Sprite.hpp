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
#ifndef NOMLIB_GRAPHICS_SPRITE_HPP
#define NOMLIB_GRAPHICS_SPRITE_HPP

#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/math/Transformable.hpp"
#include "nomlib/math/Rect.hpp"
#include "nomlib/math/Point2.hpp"
#include "nomlib/system/SDL_helpers.hpp"
#include "nomlib/graphics/sprite/SpriteSheet.hpp"
#include "nomlib/graphics/sprite/SpriteAnimator.hpp"

namespace nom {

// Forward declarations
class Texture;
class RenderWindow;
typedef const RenderWindow RenderTarget;

/// \brief Base class for bitmap objects
class Sprite: public Transformable
{
  public:
    typedef Sprite self_type;
    typedef Transformable derived_class;

    Sprite();

    /// \brief Copy constructor.
    ///
    /// \remarks The entire sprite state (texture, position, size, sprite
    /// sheet, current frame) and embedded SpriteAnimator (clips, playback
    /// state) are deep-copied.  The animator is automatically rebound to the
    /// new Sprite instance.
    Sprite( const Sprite& other );

    /// \brief Copy assignment operator.
    Sprite& operator=( const Sprite& other );

    virtual ~Sprite();

    /// \brief Construct a sprite from a dimension and color.
    ///
    /// \params color The color to fill the sprite bounds with.
    /// \params dims The width and height, in pixels, of the sprite.
    ///
    /// \returns Boolean TRUE on successful construction, or boolean FALSE when
    /// construction has failed, such as when the texture is invalid, or a
    /// failure to allocate the necessary memory.
    ///
    /// \remarks The position of the sprite will be initialized to
    /// Point2i::zero.
    bool init_with_color(const Color4i& color, const Size2i& dims);

    /// \brief Re-implements the IObject::type method.
    ///
    /// \remarks This uniquely identifies the object's type.
    ObjectTypeInfo type() const override;

    /// \brief Re-implements Transformable::set_position.
    virtual void set_position(const Point2i& pos) override;

    /// \brief Re-implements Transformable::set_size.
    virtual void set_size(const Size2i& dims) override;

    /// \brief Implements the required IDrawable::clone method.
    std::unique_ptr<Sprite> clone() const;

    std::shared_ptr<Texture> texture() const;

    bool valid() const;

    /// \brief Get the texture color of the sprite.
    Color4i color() const;

    /// \brief Get the color blend mode of the sprite.
    BlendMode color_blend_mode() const;

    /// \brief Get the alpha value of the sprite.
    uint8 alpha() const;

    /// \brief Construct a sprite from an existing texture source.
    ///
    /// \params tex An existing, valid nom::Texture reference.
    ///
    /// \returns Boolean TRUE on successful construction, or boolean FALSE when
    /// construction has failed, such as when the texture is invalid, or a
    /// failure to allocate the necessary memory.
    ///
    /// \remarks The passed in nom::Texture instance **must** outlive the
    /// destruction of this sprite!
    bool set_texture(Texture& tex);

    /// \brief Construct a sprite from an existing texture source.
    ///
    /// \params tex An existing, valid nom::Texture pointer.
    ///
    /// \returns Boolean TRUE on successful construction, or boolean FALSE when
    /// construction has failed, such as when the texture is invalid, or a
    /// failure to allocate the necessary memory.
    ///
    /// \remarks The ownership of the pointer is transferred to this sprite.
    bool set_texture(Texture* tex);

    /// \brief Construct a sprite from an existing texture source.
    ///
    /// \params tex An existing, valid nom::Texture pointer.
    ///
    /// \returns Boolean TRUE on successful construction, or boolean FALSE when
    /// construction has failed, such as when the texture is invalid, or a
    /// failure to allocate the necessary memory.
    bool set_texture(std::shared_ptr<Texture>& tex);

    bool set_alpha(uint8 opacity);

    bool set_color(const Color4i& color);

    bool set_color_blend_mode(BlendMode blend);

    /// \brief Free the stored texture.
    ///
    /// \remarks This decrements the shared reference count, potentially
    /// freeing the resource.
    void release_texture();

    // -- Sprite-sheet support ----------------------------------------------

    /// \brief Use the sprite frames from an existing SpriteSheet object.
    ///
    /// \param sheet The pre-loaded sprite sheet instance to use the frames
    /// from.
    ///
    /// \remarks The dimensions of this sprite are initialized to the first
    /// frame of the sprite sheet source.  Any named animations defined in
    /// the sheet's "animations" JSON node are automatically registered with
    /// the embedded animator.
    ///
    /// \see nom::SpriteSheet::load_file, nom::SpriteAnimator::load_clips.
    virtual void set_sprite_sheet( const SpriteSheet& sheet );

    /// \brief Returns the current sheet frame ID being rendered, or -1 if the
    /// sprite is not currently visible.
    virtual int32 frame() const;

    /// \brief Returns the total number of frames available via the attached
    /// SpriteSheet, or 1 if no sheet is attached.
    virtual int32 frames() const;

    /// \brief Set the current sheet frame ID to render.
    ///
    /// \param id The frame identifier (from the sprite sheet) to use in
    /// rendering.  Passing negative one (-1) disables rendering of the sprite
    /// (useful for toggling visibility without a placeholder frame).
    ///
    /// \remarks If no sprite sheet is attached this is a no-op.
    virtual void set_frame( int32 id );

    // -- Named animation support (built-in SpriteAnimator) -----------------

    /// \brief Play a named animation clip registered with the embedded
    /// animator (either manually via animator() or automatically from a
    /// SpriteSheet's "animations" node).
    ///
    /// \returns Boolean FALSE if no clip with that name exists.
    bool play_animation( const std::string& name );

    /// \brief Stop any currently playing animation.
    void stop_animation();

    /// \brief Pause a playing animation at its current frame.
    void pause_animation();

    /// \brief Resume a paused animation.
    void resume_animation();

    /// \brief Returns true if an animation clip is currently playing.
    bool is_animation_playing() const;

    /// \brief Returns the name of the currently playing animation clip, or an
    /// empty string if no clip is playing.
    const std::string& current_animation() const;

    /// \brief Advance any currently playing animation by delta_time seconds.
    ///
    /// \remarks This is a no-op if no animation is playing.  It is called
    /// automatically from the actions system when using SpriteAnimatorAction.
    SpriteAnimator::State update_animation( real32 delta_time );

    /// \brief Direct access to the embedded SpriteAnimator for advanced usage
    /// (registering clips manually, setting completion callbacks, etc.).
    SpriteAnimator& animator();
    const SpriteAnimator& animator() const;

    // -- Drawing -----------------------------------------------------------

    virtual void draw(RenderTarget& target) const override;

    /// Draw a rotated nom::Sprite on a nom::RenderWindow
    ///
    /// \param  target  Reference to an active nom::RenderWindow
    /// \param  angle   Rotation angle in degrees
    virtual void draw(RenderTarget& target, real64 angle) const;

  protected:
    /// \brief The underlying texture for the sprite.
    std::shared_ptr<Texture> texture_;

    /// \brief Attached sprite sheet (may be empty).
    SpriteSheet sprite_sheet_;

    /// \brief Source (input) coordinates — used for sprite sheet positioning.
    IntRect offsets_;

    /// \brief The sheet's frame ID presently in use; -1 means "not visible".
    int32 sheet_id_;

  private:
    virtual void update() override;

    /// \brief Built-in animation controller.
    SpriteAnimator animator_;

    // TODO: Implement these member variables:

    // nom::SpriteBatch can begin using this immediately. Long term, I would
    // like to either re-implement some concept of rescaling, or at the very
    // minimum, let nom::Sprite always hold a reference to a value and do
    // calculations based off it from other APIs or what not.
    // Size2f scale_factor_ = 1.0f;

    // Render queues and z-sorting, oh my! Perhaps we could even just upgrade
    // nom::Transformable to Point3..?
    // real32 z_depth_;
};

/// \brief Construct a sprite from an existing texture source.
///
/// \relates ::set_texture
std::unique_ptr<Sprite>
make_unique_sprite(Texture& tex);

/// \brief Construct a sprite from an existing texture source.
///
/// \relates ::set_texture
std::unique_ptr<Sprite>
make_unique_sprite(Texture* tex);

/// \brief Construct a sprite from an existing texture source.
///
/// \relates ::set_texture
std::unique_ptr<Sprite>
make_unique_sprite(std::shared_ptr<Texture>& tex);

/// \brief Construct a sprite from an existing texture source.
///
/// \relates ::set_texture
std::shared_ptr<Sprite>
make_shared_sprite(Texture& tex);

/// \brief Construct a sprite from an existing texture source.
///
/// \relates ::set_texture
std::shared_ptr<Sprite>
make_shared_sprite(Texture* tex);

/// \brief Construct a sprite from an existing texture source.
///
/// \relates ::set_texture
std::shared_ptr<Sprite>
make_shared_sprite(std::shared_ptr<Texture>& tex);

} // namespace nom

#endif // include guard defined
