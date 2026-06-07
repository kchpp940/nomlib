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
#ifndef NOMLIB_GRAPHICS_SPRITE_BATCH_HPP
#define NOMLIB_GRAPHICS_SPRITE_BATCH_HPP

#include "nomlib/config.hpp"
#include "nomlib/graphics/sprite/Sprite.hpp"

namespace nom {

/// \brief Extended sprite rendering using sprite sheets
///
/// \note This class was formerly the only place sprite-sheet driven frame
/// animation lived.  All of that functionality has been moved down to the
/// base nom::Sprite class.  nom::SpriteBatch is now a thin compatibility
/// wrapper kept for existing code; it forwards every call to its Sprite
/// base.
///
/// \deprecated Prefer nom::Sprite directly for new code.
class SpriteBatch: public Sprite
{
  public:
    typedef SpriteBatch self_type;
    typedef Sprite derived_class;

    /// Default construct for initializing instance variables to their
    /// respective defaults.
    SpriteBatch();

    /// \brief Copy constructor.
    ///
    /// \remarks Delegates to nom::Sprite copy constructor, which handles
    /// deep-copying the sprite state and rebinding the embedded animator.
    SpriteBatch( const SpriteBatch& other );

    /// \brief Copy assignment operator.
    ///
    /// \remarks Delegates to nom::Sprite copy assignment.
    SpriteBatch& operator=( const SpriteBatch& other );

    /// Destructor.
    virtual ~SpriteBatch();

    /// \brief Re-implements the IObject::type method.
    ///
    /// \remarks This uniquely identifies the object's type.
    ObjectTypeInfo type() const override;

    /// \brief Implements the required IDrawable::clone method.
    SpriteBatch* clone() const;

    // -- Sprite-sheet & animation API (forwarded to base) ------------------
    //
    // These methods are kept for source compatibility with existing code
    // that uses nom::SpriteBatch.  Every call simply forwards to the
    // corresponding method in nom::Sprite.

    using Sprite::set_sprite_sheet;
    using Sprite::frame;
    using Sprite::frames;
    using Sprite::set_frame;

    using Sprite::play_animation;
    using Sprite::stop_animation;
    using Sprite::pause_animation;
    using Sprite::resume_animation;
    using Sprite::is_animation_playing;
    using Sprite::current_animation;
    using Sprite::update_animation;
    using Sprite::animator;

    // -- Drawing (forwarded to base) ---------------------------------------

    using Sprite::draw;
};

} // namespace nom

#endif // include guard defined
