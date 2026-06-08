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
#include "nomlib/actions/ActionPlayer.hpp"

// Forward declarations
#include "nomlib/actions/IActionObject.hpp"
#include "nomlib/actions/DispatchQueue.hpp"

namespace nom {

// Static initializations
const char* ActionPlayer::DEBUG_CLASS_NAME = "[ActionPlayer]:";

ActionPlayer::ActionPlayer() :
  player_state_(ActionPlayer::State::RUNNING)
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION, NOM_LOG_PRIORITY_VERBOSE);
}

ActionPlayer::~ActionPlayer()
{
  NOM_LOG_TRACE_PRIO(NOM_LOG_CATEGORY_TRACE_ACTION, NOM_LOG_PRIORITY_VERBOSE);
}

bool ActionPlayer::idle() const
{
  return(this->actions_.size() == 0 && this->free_list_.size() == 0);
}

nom::size_type ActionPlayer::num_actions() const
{
  return this->actions_.size();
}

ActionPlayer::State ActionPlayer::player_state() const
{
  return this->player_state_;
}

void ActionPlayer::pause()
{
  this->player_state_ = ActionPlayer::State::PAUSED;
}

void ActionPlayer::resume()
{
  this->player_state_ = ActionPlayer::State::RUNNING;
}

void ActionPlayer::stop()
{
  this->player_state_ = ActionPlayer::State::STOPPED;
}

bool ActionPlayer::action_running(const std::string& action_name) const
{
  if( action_name.empty() ) {
    return false;
  }

  auto range = this->name_index_.equal_range(action_name);
  for( auto itr = range.first; itr != range.second; ++itr ) {
    uint64 action_id = itr->second;
    if( this->actions_.find(action_id) != this->actions_.end() ) {
      return true;
    }
  }

  return false;
}

bool ActionPlayer::cancel_action(const std::string& action_name)
{
  if( action_name.empty() ) {
    return false;
  }

  bool found_any = false;
  auto range = this->name_index_.equal_range(action_name);

  std::vector<uint64> ids_to_erase;
  ids_to_erase.reserve(this->name_index_.count(action_name));
  for( auto itr = range.first; itr != range.second; ++itr ) {
    ids_to_erase.push_back(itr->second);
  }

  for( uint64 action_id : ids_to_erase ) {
    auto res = this->actions_.find(action_id);
    if( res != this->actions_.end() ) {

      this->actions_.erase(res);
      found_any = true;
    }

    // Also remove all entries for this id from the secondary name index
    auto name_range = this->name_index_.equal_range(action_name);
    for( auto nitr = name_range.first; nitr != name_range.second; ) {
      if( nitr->second == action_id ) {
        nitr = this->name_index_.erase(nitr);
      } else {
        ++nitr;
      }
    }
  }

  return found_any;
}

void
ActionPlayer::cancel_actions(const ActionPlayer::action_names& actions)
{
  for( auto itr = actions.begin(); itr != actions.end(); ++itr ) {
    this->cancel_action(*itr);
  }
}

void ActionPlayer::cancel_actions()
{
  this->free_list_.clear();
  this->name_index_.clear();
  this->actions_.clear();
}

bool ActionPlayer::run_action(const std::shared_ptr<IActionObject>& action)
{
  return this->run_action(action, nullptr);
}

bool ActionPlayer::
run_action( const std::shared_ptr<IActionObject>& action,
            const action_callback_func& completion_func )
{
  auto dispatch_queue =
    nom::create_dispatch_queue<DispatchQueue>();
  if( dispatch_queue != nullptr ) {
    return this->run_action(action, std::move(dispatch_queue), completion_func);
  }

  NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION, "Failed to enqueue action: ",
                "could not allocate memory for the dispatch queue!" );
  return false;
}

bool ActionPlayer::update(real32 delta_time)
{
  ActionPlayer::State player_state = this->player_state();
  DispatchQueue::State dispatch_running = DispatchQueue::State::IDLING;

  // Process the queue in FIFO order
  for( auto itr = this->actions_.begin(); itr != this->actions_.end(); ++itr ) {

    uint64 action_id = itr->first;
    auto action_queue = itr->second.get();

    // This is a valid condition; enqueued actions are subject to being removed
    // before their completion, i.e.: ::run_action will happily overwrite
    // existing action keys
    if( action_queue == nullptr ) {
      NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION_PLAYER, DEBUG_CLASS_NAME,
                      "enqueue erasable (NULL)", "[action_id]:", action_id );

      this->free_list_.emplace_back(itr);
    } else {

      dispatch_running = (action_queue)->update(player_state, delta_time);

      if( dispatch_running == DispatchQueue::State::IDLING ) {
        NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION_PLAYER, DEBUG_CLASS_NAME,
                        "enqueue erasable", "[action_id]:", action_id );

        this->free_list_.emplace_back(itr);
      } // end if IDLE
    } // end if action queue is valid
  } // end for loop


  // Erase the actions from the queue in LIFO order, keeping the secondary
  // name_index_ in sync with the primary actions_ map.
  while( this->free_list_.empty() == false ) {
    auto res = this->free_list_.front();
    uint64 action_id = res->first;

    NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION_PLAYER, DEBUG_CLASS_NAME,
                    "erasing action", "[action_id]:", action_id );

    // Look up and remove any name_index_ entries pointing to this id.
    // We have to scan the whole multimap since name_index_ is keyed by name.
    for( auto nitr = this->name_index_.begin();
         nitr != this->name_index_.end(); )
    {
      if( nitr->second == action_id ) {
        nitr = this->name_index_.erase(nitr);
      } else {
        ++nitr;
      }
    }

    this->actions_.erase(res);
    this->free_list_.pop_front();
  }

  if( this->actions_.empty() == true ) {
    // Finished update iterations; all actions are completed
    return false;
  }

  // Not finished with update iterations; one or more actions are still running
  return true;
}

// Private scope

bool ActionPlayer::
run_action( const std::shared_ptr<IActionObject>& action,
            std::unique_ptr<DispatchQueue> dispatch_queue,
            const action_callback_func& completion_func )
{
  if( action == nullptr ) {
    NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                  "Could not enqueue the action -- action was NULL." );
    return false;
  }

  if( dispatch_queue == nullptr ) {
    NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                  "Could not enqueue the action -- dispatch queue was NULL." );
    return false;
  }

  // Authoritative key: the process-wide unique id generated in IActionObject's
  // constructor.  Cloned actions always get a fresh id and therefore never
  // collide with their source, even when name() is identical.
  uint64 action_uid = action->id();
  const std::string& action_name = action->name();

  if( dispatch_queue->enqueue_action(action, completion_func) == false ) {
    return false;
  }

  // NOTE: This logging category is disabled by default
  if( this->actions_.find(action_uid) != this->actions_.end() ) {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_ACTION_PLAYER,
                  "Another action with the same uid exists -- overwriting",
                  "with uid=", action_uid, ", name=", action_name );
  }

  // If the user assigned a name, register it in the secondary index so that
  // action_running / cancel_action still work by human-readable label.
  if( action_name.length() > 0 ) {
    this->name_index_.emplace(action_name, action_uid);
  }

  this->actions_[action_uid] = std::move(dispatch_queue);

  return true;
}

} // namespace nom
