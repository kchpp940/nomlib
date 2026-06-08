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

  // Deterministic teardown via the single authoritative removal path.
  // Collect the ids first because remove_action_by_id() erases from actions_
  // and we don't want to invalidate iterators mid-loop.
  std::vector<uint64> ids;
  ids.reserve(this->actions_.size());
  for( auto& kv : this->actions_ ) {
    ids.push_back(kv.first);
  }
  for( uint64 action_id : ids ) {
    this->remove_action_by_id(action_id);
  }

  // remove_action_by_id does not touch free_list_, so clear it explicitly.
  // The iterators stored there are about to become invalid anyway when the
  // map is fully emptied.
  this->free_list_.clear();
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

  // Collect all ids currently registered under this name.  We copy them out
  // first because remove_action_by_id() erases from name_index_ and would
  // invalidate our iterators if we tried to erase while walking the range.
  std::vector<uint64> ids_to_erase;
  auto range = this->name_index_.equal_range(action_name);
  ids_to_erase.reserve(std::distance(range.first, range.second));
  for( auto itr = range.first; itr != range.second; ++itr ) {
    ids_to_erase.push_back(itr->second);
  }

  bool found_any = false;
  for( uint64 action_id : ids_to_erase ) {
    // remove_action_by_id is idempotent and handles the case where the same
    // id was registered under several names (duplicate call is a no-op).
    auto before = this->actions_.size();
    this->remove_action_by_id(action_id);
    if( this->actions_.size() < before ) {
      found_any = true;
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

  // Iterate via a copied id list so remove_action_by_id() can erase from
  // actions_ without invalidating our loop iterator.
  std::vector<uint64> ids;
  ids.reserve(this->actions_.size());
  for( auto& kv : this->actions_ ) {
    ids.push_back(kv.first);
  }
  for( uint64 action_id : ids ) {
    this->remove_action_by_id(action_id);
  }

  // Defensive: remove_action_by_id should have emptied both, but clear any
  // stragglers in case of invariant breakage.
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


  // Erase the actions from the free list.  remove_action_by_id() handles the
  // three-step teardown (final_release -> drop name index entries -> erase
  // from actions_) in the correct order, and is idempotent so it's safe even
  // if the same id somehow ended up in free_list_ twice.
  while( this->free_list_.empty() == false ) {
    auto res = this->free_list_.front();
    uint64 action_id = res->first;

    NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_ACTION_PLAYER, DEBUG_CLASS_NAME,
                    "erasing action", "[action_id]:", action_id );

    this->remove_action_by_id(action_id);
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

void ActionPlayer::remove_action_by_id(uint64 action_id)
{
  auto res = this->actions_.find(action_id);
  if( res == this->actions_.end() ) {
    // Idempotent: nothing to do if the action is already gone (or never
    // existed).  Also catch the case where cancel_actions(name) hit the same
    // id twice through different name aliases, etc.
    return;
  }

  // 1. Final-release while the object is still fully alive so the derived
  //    vtable is intact and the virtual release() hook dispatches correctly.
  if( res->second != nullptr ) {
    res->second->final_release_all();
  }

  // 2. Drop *every* name_index_ entry that points at this id.  We scan the
  //    whole multimap; in practice the number of concurrently-running actions
  //    is small enough that this is fine, and correctness trumps micro-
  //    optimisation here.  A more cache-friendly structure (e.g. a bimap or
  //    an additional id->name reverse index) could be added later if profiling
  //    shows this matters.
  for( auto nitr = this->name_index_.begin();
       nitr != this->name_index_.end(); )
  {
    if( nitr->second == action_id ) {
      nitr = this->name_index_.erase(nitr);
    } else {
      ++nitr;
    }
  }

  // 3. Erase from the primary map; this destroys the unique_ptr<DispatchQueue>
  //    which runs ~DispatchQueue (a no-op in terms of resource release — all
  //    the real work was already done in step 1).
  this->actions_.erase(res);
}

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

  // If an action with this uid already exists, tear it down cleanly via the
  // single authoritative removal path before we overwrite it.  Without this
  // step the old DispatchQueue unique_ptr would simply be destroyed on
  // operator=, which would skip final_release() and leak owned resources —
  // and its name_index_ entries would also be left dangling.
  if( this->actions_.find(action_uid) != this->actions_.end() ) {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_ACTION_PLAYER,
                  "Another action with the same uid exists -- overwriting",
                  "with uid=", action_uid, ", name=", action_name );
    this->remove_action_by_id(action_uid);
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
