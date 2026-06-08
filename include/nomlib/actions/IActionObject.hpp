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
#ifndef NOMLIB_ACTIONS_IACTION_OBJECT_HPP
#define NOMLIB_ACTIONS_IACTION_OBJECT_HPP

#include <functional>
#include <vector>

#include "nomlib/config.hpp"
#include "nomlib/system/Timer.hpp"

namespace nom {

/// \brief Pure virtual base class interface for actions.
class IActionObject
{
  public:
    typedef IActionObject self_type;

    /// \brief A function pointer to a timing mode.
    ///
    /// \see nom::Linear, nom::Quad, nom::Cubic, nom::Quart, nom::Quint,
    /// nom::Back, nom::Bounce, nom::Circ, nom::Elastic, nom::Expo, nom::Sine
    typedef
    std::function<real32(real32, real32, real32, real32)>  timing_curve_func;

    /// \brief The update status of the action.
    enum FrameState
    {
      /// The action has finished its update loop.
      COMPLETED,
      /// The action is still updating; duration remains.
      PLAYING,
    };

    /// \brief The lifecycle state of the action, used for consistent
    ///        state transitions in pause/resume/rewind/release.
    enum LifecycleState
    {
      /// The action is freshly constructed or rewound, ready to start.
      IDLE,
      /// The action is currently executing (timer running).
      RUNNING,
      /// The action has been paused.
      PAUSED,
      /// The action has completed its execution.
      FINISHED,
      /// The action has released its external resources and is invalid.
      RELEASED,
    };

    IActionObject();

    virtual ~IActionObject();

    /// \brief Get the unique identifier of the action.
    const std::string& name() const;

    /// \brief Get the duration of the action.
    ///
    /// \returns The duration in fractional seconds.
    real32 duration() const;

    /// \brief Get the speed modifier of the action.
    ///
    /// \returns The speed factor of the action.
    ///
    /// \remarks A value of zero (0.0f) stops the action from progressing
    /// forward in time.
    real32 speed() const;

    /// \brief Get the timing mode function used by the action.
    ///
    /// \returns The function pointer to the timing curve function.
    ///
    /// \see nom::IActionObject::timing_curve_func
    const IActionObject::timing_curve_func& timing_curve() const;

    /// \brief Get the current lifecycle state of the action.
    ///
    /// \see nom::IActionObject::LifecycleState
    IActionObject::LifecycleState lifecycle_state() const;

    /// \brief Set the unique identifier of the action.
    void set_name(const std::string& action_id);

    /// \brief Set the speed factor of the action.
    virtual void set_speed(real32 speed);

    /// \brief Set the timing mode of the action.
    ///
    /// \see nom::IActionObject::timing_curve_func
    virtual void set_timing_curve(const IActionObject::timing_curve_func& mode);

    /// \brief Create a deep copy instance of the action in its initial state.
    ///
    /// \returns A new action instance with:
    ///   - Construction parameters fully copied
    ///   - Shared target objects (Sprite, SpriteBatch, etc.) keep shared ownership
    ///   - All runtime state (timer, elapsed_frames, iterators, initial_* values)
    ///     reset to defaults as if freshly constructed
    ///   - Name suffixed with "_cloned" to avoid key collision in ActionPlayer
    ///
    /// \note External target objects (sprites, audio buffers) are NOT deep-copied;
    ///       the cloned action shares them with the original via shared_ptr /
    ///       observer pointer semantics.  Owned resources (e.g. internally
    ///       allocated audio buffers, file handles) MUST be cloned or
    ///       re-acquired by the derived action.
    virtual std::unique_ptr<IActionObject> clone() const = 0;

    /// \brief Play the action forward in time by one time step.
    ///
    /// \param delta_time Reserved for application-defined implementations.
    ///
    /// \note <b>This method should not normally need to be called externally!
    /// Exceptions might include: a) implementing a new action; b) advanced
    /// debugging</b>
    ///
    /// \post If the action reaches its end, FrameState::COMPLETED is returned
    ///       and lifecycle_state() becomes FINISHED.
    ///
    /// \see nom::DispatchQueue
    virtual IActionObject::FrameState next_frame(real32 delta_time) = 0;

    /// \brief Play the action backwards in time by one time step.
    ///
    /// \param delta_time Reserved for application-defined implementations.
    ///
    /// \note <b>This method should not normally need to be called externally!
    /// Exceptions might include: a) implementing a new action; b) advanced
    /// debugging</b>
    ///
    /// \remarks Not all actions are reversible -- see the action's
    /// documentation for its implementation details.  Non-reversible actions
    /// should behave identically to next_frame().
    ///
    /// \see nom::ReversedAction
    virtual IActionObject::FrameState prev_frame(real32 delta_time) = 0;

    /// \brief Freeze the action's internal state and any held external resources.
    ///
    /// \param delta_time Reserved for application-defined implementations.
    ///
    /// \note <b>This method should not normally need to be called externally!
    /// Exceptions might include: a) implementing a new action; b) advanced
    /// debugging</b>
    ///
    /// \post lifecycle_state() == PAUSED
    /// \post The internal timer is stopped; any playing audio / animation is
    ///       also paused at the current position.
    ///
    /// \see nom::DispatchQueue
    virtual void pause(real32 delta_time);

    /// \brief Resume the internal state of the action from where it was paused.
    ///
    /// \param delta_time Reserved for application-defined implementations.
    ///
    /// \note <b>This method should not normally need to be called externally!
    /// Exceptions might include: a) implementing a new action; b) advanced
    /// debugging</b>
    ///
    /// \post lifecycle_state() == RUNNING (if the action was not already FINISHED)
    ///
    /// \see nom::DispatchQueue
    virtual void resume(real32 delta_time);

    /// \brief Reset the internal state of the action back to its initial
    /// starting values, making it safe to replay from the beginning.
    ///
    /// \param delta_time Reserved for application-defined implementations.
    ///
    /// \note <b>This method should not normally need to be called externally!
    /// Exceptions might include: a) implementing a new action; b) advanced
    /// debugging</b>
    ///
    /// \post lifecycle_state() == IDLE
    /// \post Timer is reset; elapsed_frames_ is zero; all internal iterators
    ///       and counters are at their construction defaults; "first frame"
    ///       flags are cleared so that first_frame() will fire again on the
    ///       next next_frame() call.
    /// \post Target objects are restored to their recorded initial state
    ///       (position, frame, volume, etc.) when applicable.
    ///
    /// \see nom::RepeatForAction, nom::RepeatForeverAction
    virtual void rewind(real32 delta_time);

    /// \brief Free externally referenced resources held by the action.
    ///
    /// \note <b>This method should not normally need to be called externally!
    /// Exceptions might include: a) implementing a new action; b) advanced
    /// debugging</b>
    ///
    /// \details
    ///   - shared_ptr-held targets (Sprite, SpriteBatch, ...): reference is released.
    ///   - Action-owned raw resources (internally allocated SoundBuffers, file
    ///     handles, etc.): explicitly freed / deleted.
    ///   - Non-owning observer pointers: cleared to nullptr (NOT deleted).
    ///
    /// \post lifecycle_state() == RELEASED
    /// \post Calling next_frame / prev_frame after release() is safe and
    ///       immediately returns COMPLETED.
    ///
    /// \see nom::RemoveAction
    virtual void release();

  protected:
    /// \brief Get the current state of the action.
    ///
    /// \see nom::IActionObject::FrameState
    IActionObject::FrameState status() const;

    /// \brief Set the duration of the action.
    ///
    /// \param seconds The duration in seconds.
    void set_duration(real32 seconds);

    /// \brief Set the state of the action.
    ///
    /// \param state One of the IActionObject::FrameState enumeration values.
    void set_status(FrameState state);

    /// \brief Set the lifecycle state of the action.
    ///
    /// \param state One of the IActionObject::LifecycleState enumeration values.
    void set_lifecycle_state(LifecycleState state);

    /// \brief Internal frames counter.
    ///
    /// \remarks This is intended purely for debugging convenience.
    real32 elapsed_frames_ = 0.0f;

    /// \brief Internal time clock (milliseconds resolution).
    ///
    /// \remarks Each action is responsible for keeping track of its clock --
    /// this provides a stable, fixed duration that yields to reliable results
    /// across any frame rate -- at the cost of potentially skipping frames
    /// when performance suffers.
    Timer timer_;

  private:
    FrameState status_ = FrameState::PLAYING;
    LifecycleState lifecycle_state_ = LifecycleState::IDLE;
    std::string name_;
    real32 duration_ = 0.0f;
    real32 speed_ = 1.0f;
    timing_curve_func timing_curve_ = nullptr;
};

/// \brief A collection of actions.
///
/// \relates nom::IActionObject
typedef std::vector<std::shared_ptr<IActionObject>> action_list;

/// \brief Constructor function for creating an action.
///
/// \param args The arguments to pass to the constructed object.
///
/// \relates nom::IActionObject
template<typename ObjectType, typename... ObjectArgs>
std::shared_ptr<ObjectType> create_action(ObjectArgs&&... args)
{
  // IMPORTANT: We risk object slicing if we use std::make_shared here! The
  // problem occurs when the end-user tries to return the created action
  // pointer by value.
  return( std::shared_ptr<ObjectType>(
          new ObjectType( std::forward<ObjectArgs>(args) ... ) ) );
}

/// \brief Constructor function for creating an action that uses a collection
/// of actions.
///
/// \param actions The collection of actions to pass to the constructed object.
///
/// \remarks The collection can be constructed in-place with a
/// [std::initializer_list](http://en.cppreference.com/w/cpp/utility/initializer_list)
/// object.
///
/// \note If you are building with Visual Studio 2013 on the Windows platform,
/// you will want to ensure that you have Update 2 or better applied before
/// using this function call with a std::initializer_list object. See also:
/// [stackoverflow.com: std::shared_ptr in an std::initializer_list appears to be getting destroyed prematurely](http://stackoverflow.com/questions/22924358/stdshared-ptr-in-an-stdinitializer-list-appears-to-be-getting-destroyed-prem/22924473#22924473)
///
/// \see nom::GroupAction, nom::SequenceAction
///
/// \relates nom::IActionObject
template<typename ObjectType>
std::shared_ptr<ObjectType>
create_action(const action_list& actions)
{
  // IMPORTANT: We risk object slicing if we use std::make_shared here! The
  // problem occurs when the end-user tries to return the created action
  // pointer by value.
  return( std::shared_ptr<ObjectType>( new ObjectType(actions) ) );
}

} // namespace nom

#endif // include guard defined

/// \class nom::IActionObject
/// \ingroup actions
///
/// **TODO:** This documentation section is a *STUB*!
///
/// ## Creating Custom Actions
///
/// A simple, bare-bones example:
/// \snippet src/actions/WaitForDurationAction.cpp creating_custom_actions
///
/// # References (Conceptual)
/// \see [SpriteKit: SKAction Class Reference](https://developer.apple.com/library/prerelease/ios/documentation/SpriteKit/Reference/SKAction_Ref/index.html)
/// \see [SpriteKit: Creating Actions That Run Other Actions](https://developer.apple.com/library/ios/documentation/GraphicsAnimation/Conceptual/SpriteKit_PG/AddingActionstoSprites/AddingActionstoSprites.html)
///
