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
class SpriteAnimator;

/// \brief Drive a nom::SpriteAnimator through the actions update loop
///
/// \remarks This action plays a single named animation clip on a
/// nom::SpriteAnimator.  For looping clips the action will report
/// FrameState::PLAYING indefinitely — wrap it with nom::RepeatForeverAction
/// or nom::RepeatForAction as appropriate. For non-looping clips the action
/// will report FrameState::COMPLETED once the clip finishes, and the
/// SpriteAnimator's completion callback (if any) will fire naturally.
///
/// \see nom::SpriteAnimator, nom::SpriteAnimationClip
class SpriteAnimatorAction: public virtual IActionObject
{
  public:
    typedef SpriteAnimatorAction self_type;
    typedef IActionObject derived_type;

    /// \brief Play a named clip on a SpriteAnimator via the actions system.
    ///
    /// \param animator   The animator to drive. Must outlive this action.
    /// \param clip_name  Name of a previously registered clip.
    SpriteAnimatorAction( const std::shared_ptr<SpriteAnimator>& animator,
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

    /// \brief The animator we are driving.
    std::shared_ptr<SpriteAnimator> animator_;

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
/// \brief This action bridges nom::SpriteAnimator into the existing action
/// system so that animations can be sequenced with other actions like
/// nom::MoveByAction, nom::CallbackAction, etc.
///
/// Typical usage:
/// \code
///   auto animator = std::make_shared<nom::SpriteAnimator>(sprite_batch);
///   animator->add_clip(nom::SpriteAnimationClip("attack", 0, 5, 24.0f));
///
///   auto attack_action = nom::create_action<nom::SpriteAnimatorAction>(
///       animator, "attack");
///
///   actions.run_action(nom::create_action<nom::SequenceAction>(
///       nom::action_list{ attack_action, return_to_idle }));
/// \endcode
///
