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
#include "nomlib/system/InputMapper/InputActionProfileManager.hpp"

#include <algorithm>
#include <sstream>

#include "nomlib/core/err.hpp"
#include "nomlib/ptree.hpp"
#include "nomlib/system/EventHandler.hpp"
#include "nomlib/system/InputMapper/InputAction.hpp"
#include "nomlib/system/InputMapper/InputActionMapper.hpp"

namespace nom {

namespace {

real32 normalize_axis(int16 raw_value)
{
  if( raw_value > 0 ) {
    return static_cast<real32>(raw_value) / 32767.0f;
  } else if( raw_value < 0 ) {
    return static_cast<real32>(raw_value) / 32768.0f;
  }
  return 0.0f;
}

real32 axis_magnitude(real32 normalized, AxisDirection direction)
{
  if( direction == AxisDirection::Positive ) {
    return std::max(0.0f, normalized);
  } else {
    return std::max(0.0f, -normalized);
  }
}

} // anonymous namespace

struct RawBindingState
{
  bool raw_held = false;
  real32 raw_value = 0.0f;
  const InputActionBinding* binding = nullptr;
};

typedef std::map<std::string, std::vector<RawBindingState>> RawActionStateMap;
typedef std::map<int, RawActionStateMap> RawPlayerStateMap;

// Private implementation data
struct InputActionProfileManager::Impl
{
  RawPlayerStateMap raw_states;
};

InputActionProfileManager::InputActionProfileManager()
  : impl_(new Impl())
{
}

InputActionProfileManager::~InputActionProfileManager()
{
  delete this->impl_;
}

std::string InputActionProfileManager::player_state_name(int player_index)
{
  std::ostringstream oss;
  oss << "__iap_player_" << player_index;
  return oss.str();
}

bool InputActionProfileManager::load_profile(const std::string& name,
                                             const Value& root)
{
  auto profile = make_shared_input_action_profile();
  if( profile->load_from_value(root) == false ) {
    return false;
  }
  if( profile->validate() == false ) {
    NOM_LOG_WARN( NOM, "Profile '" + name + "' contains invalid bindings" );
  }
  if( profile->name().empty() ) {
    profile->set_name(name);
  }
  this->profiles_[name] = profile;
  return true;
}

bool InputActionProfileManager::add_profile(
    const std::string& name,
    std::shared_ptr<InputActionProfile> profile)
{
  if( profile == nullptr ) {
    return false;
  }
  this->profiles_[name] = profile;
  return true;
}

void InputActionProfileManager::remove_profile(const std::string& name)
{
  auto it = this->profiles_.find(name);
  if( it != this->profiles_.end() ) {
    for( auto pit = this->player_profiles_.begin();
         pit != this->player_profiles_.end(); ++pit ) {
      if( pit->second == name ) {
        std::string sname = this->player_state_name(pit->first);
        this->state_mapper_.disable(sname);
      }
    }
    this->profiles_.erase(it);
  }
}

bool InputActionProfileManager::has_profile(const std::string& name) const
{
  return this->profiles_.find(name) != this->profiles_.end();
}

std::shared_ptr<InputActionProfile>
InputActionProfileManager::profile(const std::string& name) const
{
  auto it = this->profiles_.find(name);
  if( it == this->profiles_.end() ) {
    return nullptr;
  }
  return it->second;
}

std::vector<std::string> InputActionProfileManager::profile_names() const
{
  std::vector<std::string> names;
  names.reserve(this->profiles_.size());
  for( auto it = this->profiles_.begin(); it != this->profiles_.end(); ++it ) {
    names.push_back(it->first);
  }
  return names;
}

bool InputActionProfileManager::set_profile(int player_index,
                                            const std::string& profile_name)
{
  if( this->profiles_.find(profile_name) == this->profiles_.end() ) {
    NOM_LOG_ERR( NOM, "Unknown profile: " + profile_name );
    return false;
  }

  this->player_profiles_[player_index] = profile_name;
  this->rebuild_player_state(player_index);
  return true;
}

std::string InputActionProfileManager::active_profile(int player_index) const
{
  auto it = this->player_profiles_.find(player_index);
  if( it == this->player_profiles_.end() ) {
    return std::string();
  }
  return it->second;
}

bool InputActionProfileManager::set_player_device(int player_index,
                                                  JoystickID device_id)
{
  auto conflicts = this->find_device_conflicts();
  for( auto it = conflicts.begin(); it != conflicts.end(); ++it ) {
    if( it->second == device_id && it->first != player_index ) {
      std::ostringstream oss;
      oss << "Device ID " << device_id << " is already assigned to player "
          << it->first;
      NOM_LOG_WARN( NOM, oss.str() );
    }
  }

  this->player_devices_[player_index] = device_id;
  this->rebuild_player_state(player_index);
  return true;
}

JoystickID InputActionProfileManager::player_device(int player_index) const
{
  auto it = this->player_devices_.find(player_index);
  if( it == this->player_devices_.end() ) {
    return -1;
  }
  return it->second;
}

std::vector<std::pair<int, JoystickID>>
InputActionProfileManager::find_device_conflicts() const
{
  std::vector<std::pair<int, JoystickID>> result;
  std::map<JoystickID, int> device_to_player;

  for( auto it = this->player_devices_.begin();
       it != this->player_devices_.end(); ++it ) {
    JoystickID dev = it->second;
    if( dev < 0 ) continue;

    auto found = device_to_player.find(dev);
    if( found != device_to_player.end() ) {
      result.push_back(std::make_pair(found->second, dev));
      result.push_back(std::make_pair(it->first, dev));
    } else {
      device_to_player[dev] = it->first;
    }
  }

  return result;
}

void InputActionProfileManager::set_event_handler(EventHandler& evt_handler)
{
  this->state_mapper_.set_event_handler(evt_handler);
}

void InputActionProfileManager::rebuild_player_state(int player_index)
{
  std::string sname = this->player_state_name(player_index);
  this->state_mapper_.disable(sname);
  this->state_mapper_.erase(sname);

  auto profile_it = this->player_profiles_.find(player_index);
  if( profile_it == this->player_profiles_.end() ) {
    return;
  }
  auto profile = this->profile(profile_it->second);
  if( profile == nullptr ) {
    return;
  }

  JoystickID device_id = this->player_device(player_index);

  this->ensure_player_state(player_index);
  RawActionStateMap& raw_map = this->impl_->raw_states[player_index];
  raw_map.clear();

  InputActionMapper mapper;
  std::vector<std::string> actions = profile->action_names();

  for( auto ait = actions.begin(); ait != actions.end(); ++ait ) {
    const std::string& action_name = *ait;
    const InputActionProfile::BindingList& bindings = profile->bindings(action_name);

    raw_map[action_name].reserve(bindings.size());
    for( size_type bidx = 0; bidx < bindings.size(); ++bidx ) {
      RawBindingState rbs;
      rbs.binding = &bindings[bidx];
      raw_map[action_name].push_back(rbs);
    }

    InputActionProfile::InputActionPtrList input_actions =
        profile->create_input_actions(action_name, device_id);

    const InputActionProfile::BindingList& blist = profile->bindings(action_name);
    size_type binding_idx = 0;
    size_type action_count = 0;

    for( auto iait = input_actions.begin(); iait != input_actions.end(); ++iait ) {
      if( binding_idx >= blist.size() ) break;
      const InputActionBinding& binding = blist[binding_idx];

      RawBindingState* rbs_ptr = &raw_map[action_name][binding_idx];

      switch( binding.type ) {
        case InputBindingType::Keyboard:
        case InputBindingType::GameControllerButton:
        case InputBindingType::JoystickButton:
        case InputBindingType::JoystickHat:
        {
          bool is_press_action = false;
          uint32 et = (*iait)->event().type;
          if( et == Event::KEY_PRESS ||
              et == Event::GAME_CONTROLLER_BUTTON_PRESS ||
              et == Event::JOYSTICK_BUTTON_PRESS ||
              et == Event::JOYSTICK_HAT_MOTION ) {
            is_press_action = true;
          }

          event_callback cb = [rbs_ptr, is_press_action, &binding](const Event& ev) {
            bool pressed_state = is_press_action;

            if( binding.type == InputBindingType::JoystickHat ) {
              if( ev.jhat.value == binding.js_hat_value &&
                  ev.jhat.value != Joystick::HAT_CENTERED ) {
                pressed_state = true;
              } else if( ev.jhat.value == Joystick::HAT_CENTERED ) {
                pressed_state = false;
              } else {
                return;
              }
            }

            rbs_ptr->raw_held = pressed_state;
            rbs_ptr->raw_value = pressed_state ? 1.0f : 0.0f;
            rbs_ptr->binding = &binding;
          };
          mapper.insert(action_name, *(*iait), cb);

          ++action_count;
          if( (binding.type == InputBindingType::Keyboard ||
               binding.type == InputBindingType::GameControllerButton ||
               binding.type == InputBindingType::JoystickButton) &&
              action_count >= 2 ) {
            action_count = 0;
            ++binding_idx;
          } else if( binding.type == InputBindingType::JoystickHat ) {
            action_count = 0;
            ++binding_idx;
          }
        } break;

        case InputBindingType::GameControllerAxis:
        case InputBindingType::JoystickAxis:
        {
          event_callback cb = [rbs_ptr, &binding](const Event& ev) {
            int16 raw_val = 0;
            if( binding.type == InputBindingType::GameControllerAxis ) {
              raw_val = ev.caxis.value;
            } else {
              raw_val = ev.jaxis.value;
            }
            real32 norm = normalize_axis(raw_val);
            real32 mag = axis_magnitude(norm, binding.axis_direction);
            real32 threshold = binding.axis_threshold;
            if( threshold <= 0.0f ) threshold = 0.2f;

            if( mag >= threshold ) {
              rbs_ptr->raw_held = true;
              rbs_ptr->raw_value = std::min(1.0f, mag);
            } else {
              rbs_ptr->raw_held = false;
              rbs_ptr->raw_value = 0.0f;
            }
            rbs_ptr->binding = &binding;
          };
          mapper.insert(action_name, *(*iait), cb);
          ++binding_idx;
        } break;

        default:
          ++binding_idx;
          break;
      }
    }
  }

  this->state_mapper_.insert(sname, mapper, true);
  this->player_state_names_[player_index] = sname;
}

void InputActionProfileManager::ensure_player_state(int player_index)
{
  if( this->player_states_.find(player_index) == this->player_states_.end() ) {
    this->player_states_[player_index] = ActionStateMap();
    this->active_bindings_[player_index].clear();
  }
  if( this->impl_->raw_states.find(player_index) == this->impl_->raw_states.end() ) {
    this->impl_->raw_states[player_index] = RawActionStateMap();
  }
}

void InputActionProfileManager::resolve_conflicts(int player_index)
{
  auto pit = this->player_states_.find(player_index);
  if( pit == this->player_states_.end() ) return;

  ActionStateMap& final_states = pit->second;
  RawActionStateMap& raw_map = this->impl_->raw_states[player_index];

  for( auto ait = raw_map.begin(); ait != raw_map.end(); ++ait ) {
    const std::string& action_name = ait->first;
    std::vector<RawBindingState>& raw_bindings = ait->second;

    if( raw_bindings.empty() ) {
      continue;
    }

    bool has_non_conflict = false;
    for( auto rit = raw_bindings.begin(); rit != raw_bindings.end(); ++rit ) {
      if( rit->raw_held && rit->binding && !rit->binding->conflict_allow ) {
        has_non_conflict = true;
        break;
      }
    }

    bool any_held = false;
    real32 max_value = 0.0f;
    bool any_blocked = false;

    for( auto rit = raw_bindings.begin(); rit != raw_bindings.end(); ++rit ) {
      if( !rit->raw_held || !rit->binding ) continue;

      if( has_non_conflict && rit->binding->conflict_allow ) {
        any_blocked = true;
        continue;
      }

      any_held = true;
      if( rit->raw_value > max_value ) {
        max_value = rit->raw_value;
      }
    }

    InputActionRuntimeState& state = final_states[action_name];
    bool prev_held = state.held;
    state.held = any_held;
    state.prev_value = state.value;
    state.value = max_value;
    state.conflict_blocked = any_blocked;

    if( any_held && !prev_held ) {
      state.pressed = true;
    }
    if( !any_held && prev_held ) {
      state.released = true;
    }
  }
}

void InputActionProfileManager::reset_frame_states(int player_index)
{
  auto pit = this->player_states_.find(player_index);
  if( pit == this->player_states_.end() ) return;

  for( auto ait = pit->second.begin(); ait != pit->second.end(); ++ait ) {
    ait->second.pressed = false;
    ait->second.released = false;
  }
}

void InputActionProfileManager::update()
{
  for( auto pit = this->player_states_.begin();
       pit != this->player_states_.end(); ++pit ) {
    this->reset_frame_states(pit->first);
    this->resolve_conflicts(pit->first);
  }
}

bool InputActionProfileManager::is_pressed(int player_index,
                                           const std::string& action) const
{
  const InputActionRuntimeState* st = this->action_state(player_index, action);
  return st ? st->pressed : false;
}

bool InputActionProfileManager::is_released(int player_index,
                                            const std::string& action) const
{
  const InputActionRuntimeState* st = this->action_state(player_index, action);
  return st ? st->released : false;
}

bool InputActionProfileManager::is_held(int player_index,
                                        const std::string& action) const
{
  const InputActionRuntimeState* st = this->action_state(player_index, action);
  return st ? st->held : false;
}

real32 InputActionProfileManager::action_value(int player_index,
                                               const std::string& action) const
{
  const InputActionRuntimeState* st = this->action_state(player_index, action);
  return st ? st->value : 0.0f;
}

const InputActionRuntimeState*
InputActionProfileManager::action_state(int player_index,
                                        const std::string& action) const
{
  auto pit = this->player_states_.find(player_index);
  if( pit == this->player_states_.end() ) return nullptr;

  auto ait = pit->second.find(action);
  if( ait == pit->second.end() ) return nullptr;

  return &ait->second;
}

bool InputActionProfileManager::is_pressed(const std::string& action) const
{
  return this->is_pressed(DEFAULT_PLAYER, action);
}

bool InputActionProfileManager::is_released(const std::string& action) const
{
  return this->is_released(DEFAULT_PLAYER, action);
}

bool InputActionProfileManager::is_held(const std::string& action) const
{
  return this->is_held(DEFAULT_PLAYER, action);
}

real32 InputActionProfileManager::action_value(const std::string& action) const
{
  return this->action_value(DEFAULT_PLAYER, action);
}

void InputActionProfileManager::clear_states()
{
  this->player_states_.clear();
  this->impl_->raw_states.clear();
  this->active_bindings_.clear();
}

InputStateMapper& InputActionProfileManager::state_mapper()
{
  return this->state_mapper_;
}

const InputStateMapper& InputActionProfileManager::state_mapper() const
{
  return this->state_mapper_;
}

std::unique_ptr<InputActionProfileManager> make_unique_input_action_profile_manager()
{
  return std::unique_ptr<InputActionProfileManager>(new InputActionProfileManager());
}

std::shared_ptr<InputActionProfileManager> make_shared_input_action_profile_manager()
{
  return std::shared_ptr<InputActionProfileManager>(new InputActionProfileManager());
}

} // namespace nom
