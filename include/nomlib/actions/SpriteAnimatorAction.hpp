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
#ifndef NOMLIB_ACTIONS_SPRITE_ANIMATOR_ACTION_HPP
#define NOMLIB_ACTIONS_SPRITE_ANIMATOR_ACTION_HPP

#include <memory>
#include <string>

#include "nomlib/config.hpp"
#include "nomlib/actions/IActionObject.hpp"

namespace nom {

// Forward declarations
class SpriteBatch;

/// \brief Drive a nom::SpriteBatch's built-in animator through the actions update loop
///
/// \remarks This action plays a single named animation clip on a
/// nom::SpriteBatch via its object-level play_animation / update_animation
/// entry points.  For looping clips the action will report
/// FrameState::PLAYING indefinitely — wrap it with nom::RepeatForeverAction
/// or nom::RepeatForAction as appropriate. For non-looping clips the action
/// will report FrameState::COMPLETED once the clip finishes, and the
/// SpriteAnimator's completion callback (if any) will fire naturally.
///
/// \see nom::SpriteBatch, nom::SpriteAnimator, nom::SpriteAnimationClip
class SpriteAnimatorAction: public virtual IActionObject
{
  public:
    typedef SpriteAnimatorAction self_type;
    typedef IActionObject derived_type;

    /// \brief Play a named clip on a SpriteBatch via the actions system.
    ///
    /// \param drawable   The target sprite. Must outlive this action.
    /// \param clip_name  Name of a previously registered animation clip.
    SpriteAnimatorAction( const std::shared_ptr<SpriteBatch>& drawable,
                          const std::string& clip_name );

    virtual ~SpriteAnimatorAction();

    virtual std::unique_ptr<IActionObject> clone() const override;

    virtual IActionObject::FrameState next_frame( real32 delta_time ) override;

    virtual IActionObject::FrameState prev_frame( real32 delta_time ) override;

    virtual void pause( real32 delta_time ) override;

    virtual void resume( real32 delta_time ) override;

    virtual void rewind( real32 delta_time ) override;

    virtual void release() override;

  private:
    static const char* DEBUG_CLASS_NAME;

    /// \brief The sprite we are driving.
    std::shared_ptr<SpriteBatch> drawable_;

    /// \brief Clip name requested at construction time.
    std::string clip_name_;

    /// \brief Whether the clip was started on first update.
    bool started_;
};

} // namespace nom

#endif // include guard defined

/// \class nom::SpriteAnimatorAction
/// \ingroup actions
///
/// \brief This action bridges nom::SpriteBatch's built-in animator into the
/// existing action system so that animations can be sequenced with other
/// actions like nom::MoveByAction, nom::CallbackAction, etc.
///
/// Typical usage:
/// \code
///   auto sprite = std::make_shared<nom::SpriteBatch>();
///   sprite->set_sprite_sheet(preloaded_sheet); // sheet has animations defined
///
///   auto attack_action = nom::create_action<nom::SpriteAnimatorAction>(
///       sprite, "attack");
///
///   actions.run_action(nom::create_action<nom::SequenceAction>(
///       nom::action_list{ attack_action, return_to_idle }));
/// \endcode
///
