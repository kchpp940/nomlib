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

#include "nomlib/core/err.hpp"
#include "nomlib/system/EventHandler.hpp"
#include "nomlib/ptree.hpp"

namespace nom {

namespace {

const real32 AXIS_MIN = -32768.0f;
const real32 AXIS_MAX = 32767.0f;

real32 normalize_axis(int16 raw_value)
{
  if( raw_value >= 0 ) {
    return static_cast<real32>(raw_value) / AXIS_MAX;
  } else {
    return static_cast<real32>(raw_value) / -AXIS_MIN;
  }
}

real32 axis_magnitude(int16 raw_value, AxisDirection dir)
{
  real32 normalized = normalize_axis(raw_value);
  if( dir == AxisDirection::Positive ) {
    return normalized > 0.0f ? normalized : 0.0f;
  } else {
    return normalized < 0.0f ? -normalized : 0.0f;
  }
}

} // anonymous namespace

InputActionProfileManager::InputActionProfileManager()
{
}

InputActionProfileManager::~InputActionProfileManager()
{
}

bool InputActionProfileManager::load_profile(const std::string& name,
                                             const Value& root)
{
  auto profile = make_shared_input_action_profile();
  if( profile->load_from_value(root) == false ) {
    return false;
  }
  if( profile->name().empty() ) {
    profile->set_name(name);
  }
  this->profiles_[name] = profile;
  return true;
}

bool InputActionProfileManager::add_profile(const std::string& name,
                                            std::shared_ptr<InputActionProfile> profile)
{
  if( profile == nullptr ) {
    return false;
  }
  if( profile->name().empty() ) {
    profile->set_name(name);
  }
  this->profiles_[name] = profile;
  return true;
}

void InputActionProfileManager::remove_profile(const std::string& name)
{
  this->profiles_.erase(name);
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
    NOM_LOG_ERR( NOM, "Could not set profile: profile '" + profile_name + "' not found" );
    return false;
  }
  this->player_profiles_[player_index] = profile_name;
  this->ensure_player_state(player_index);
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

void InputActionProfileManager::set_player_device(int player_index,
                                                  JoystickID device_id)
{
  this->player_devices_[player_index] = device_id;
}

JoystickID InputActionProfileManager::player_device(int player_index) const
{
  auto it = this->player_devices_.find(player_index);
  if( it == this->player_devices_.end() ) {
    return -1;
  }
  return it->second;
}

void InputActionProfileManager::set_event_handler(EventHandler& evt_handler)
{
  this->event_handler_ = &evt_handler;

  auto event_watch = nom::event_filter( [=](const Event& evt, void* data) {
    this->on_event(evt);
  });

  this->event_handler_->append_event_watch(event_watch, nullptr);
}

void InputActionProfileManager::update()
{
  for( auto it = this->player_states_.begin(); it != this->player_states_.end(); ++it ) {
    this->reset_frame_states(it->first);
  }
}

void InputActionProfileManager::reset_frame_states(int player_index)
{
  auto player_it = this->player_states_.find(player_index);
  if( player_it == this->player_states_.end() ) {
    return;
  }

  ActionStateMap& states = player_it->second;
  for( auto it = states.begin(); it != states.end(); ++it ) {
    InputActionState& s = it->second;
    s.pressed = false;
    s.released = false;
    s.prev_value = s.value;
  }
}

void InputActionProfileManager::ensure_player_state(int player_index)
{
  if( this->player_states_.find(player_index) == this->player_states_.end() ) {
    this->player_states_[player_index] = ActionStateMap();
  }

  auto profile_it = this->player_profiles_.find(player_index);
  if( profile_it == this->player_profiles_.end() ) {
    return;
  }

  auto prof = this->profile(profile_it->second);
  if( prof == nullptr ) {
    return;
  }

  ActionStateMap& states = this->player_states_[player_index];
  std::vector<std::string> names = prof->action_names();
  for( auto it = names.begin(); it != names.end(); ++it ) {
    if( states.find(*it) == states.end() ) {
      states[*it] = InputActionState();
    }
  }
}

bool InputActionProfileManager::binding_matches_device(
    const InputActionBinding& binding, int player_index) const
{
  if( binding.type == InputBindingType::Keyboard ) {
    return true;
  }

  JoystickID assigned_device = this->player_device(player_index);

  if( binding.device_id == -1 ) {
    if( assigned_device == -1 ) {
      return true;
    }
    return true;
  }

  if( assigned_device == -1 ) {
    return true;
  }

  return binding.device_id == assigned_device;
}

void InputActionProfileManager::on_event(const Event& ev)
{
  switch(ev.type)
  {
    default: break;

    case Event::KEY_PRESS:
    case Event::KEY_RELEASE:
    {
      this->process_key_event(ev);
    } break;

    case Event::GAME_CONTROLLER_BUTTON_PRESS:
    case Event::GAME_CONTROLLER_BUTTON_RELEASE:
    {
      this->process_controller_button_event(ev);
    } break;

    case Event::GAME_CONTROLLER_AXIS_MOTION:
    {
      this->process_controller_axis_event(ev);
    } break;

    case Event::JOYSTICK_BUTTON_PRESS:
    case Event::JOYSTICK_BUTTON_RELEASE:
    {
      this->process_joystick_button_event(ev);
    } break;

    case Event::JOYSTICK_AXIS_MOTION:
    {
      this->process_joystick_axis_event(ev);
    } break;

    case Event::JOYSTICK_HAT_MOTION:
    {
      this->process_joystick_hat_event(ev);
    } break;
  }
}

void InputActionProfileManager::process_key_event(const Event& ev)
{
  bool pressed = (ev.type == Event::KEY_PRESS);

  for( auto pp_it = this->player_profiles_.begin();
       pp_it != this->player_profiles_.end(); ++pp_it ) {
    int player_index = pp_it->first;
    const std::string& profile_name = pp_it->second;

    auto prof = this->profile(profile_name);
    if( prof == nullptr ) continue;

    this->ensure_player_state(player_index);

    std::vector<std::string> action_names = prof->action_names();
    for( auto a_it = action_names.begin(); a_it != action_names.end(); ++a_it ) {
      const std::string& action_name = *a_it;
      const InputActionProfile::BindingList& bindings = prof->bindings(action_name);

      for( auto b_it = bindings.begin(); b_it != bindings.end(); ++b_it ) {
        const InputActionBinding& binding = *b_it;
        if( binding.type != InputBindingType::Keyboard ) continue;
        if( this->binding_matches_device(binding, player_index) == false ) continue;

        if( binding.key_sym == ev.key.sym ) {
          if( binding.key_mod != 0 && binding.key_mod != ev.key.mod ) {
            continue;
          }
          this->apply_binding_state(player_index, action_name, binding, pressed, pressed ? 1.0f : 0.0f);
        }
      }
    }
  }
}

void InputActionProfileManager::process_controller_button_event(const Event& ev)
{
  bool pressed = (ev.type == Event::GAME_CONTROLLER_BUTTON_PRESS);

  for( auto pp_it = this->player_profiles_.begin();
       pp_it != this->player_profiles_.end(); ++pp_it ) {
    int player_index = pp_it->first;
    const std::string& profile_name = pp_it->second;

    JoystickID assigned_device = this->player_device(player_index);
    if( assigned_device != -1 && assigned_device != ev.cbutton.id ) {
      continue;
    }

    auto prof = this->profile(profile_name);
    if( prof == nullptr ) continue;

    this->ensure_player_state(player_index);

    std::vector<std::string> action_names = prof->action_names();
    for( auto a_it = action_names.begin(); a_it != action_names.end(); ++a_it ) {
      const std::string& action_name = *a_it;
      const InputActionProfile::BindingList& bindings = prof->bindings(action_name);

      for( auto b_it = bindings.begin(); b_it != bindings.end(); ++b_it ) {
        const InputActionBinding& binding = *b_it;
        if( binding.type != InputBindingType::GameControllerButton ) continue;
        if( binding.device_id != -1 && binding.device_id != ev.cbutton.id ) continue;

        if( binding.gc_button == static_cast<GameController::Button>(ev.cbutton.button) ) {
          this->apply_binding_state(player_index, action_name, binding, pressed, pressed ? 1.0f : 0.0f);
        }
      }
    }
  }
}

void InputActionProfileManager::process_controller_axis_event(const Event& ev)
{
  for( auto pp_it = this->player_profiles_.begin();
       pp_it != this->player_profiles_.end(); ++pp_it ) {
    int player_index = pp_it->first;
    const std::string& profile_name = pp_it->second;

    JoystickID assigned_device = this->player_device(player_index);
    if( assigned_device != -1 && assigned_device != ev.caxis.id ) {
      continue;
    }

    auto prof = this->profile(profile_name);
    if( prof == nullptr ) continue;

    this->ensure_player_state(player_index);

    std::vector<std::string> action_names = prof->action_names();
    for( auto a_it = action_names.begin(); a_it != action_names.end(); ++a_it ) {
      const std::string& action_name = *a_it;
      const InputActionProfile::BindingList& bindings = prof->bindings(action_name);

      for( auto b_it = bindings.begin(); b_it != bindings.end(); ++b_it ) {
        const InputActionBinding& binding = *b_it;
        if( binding.type != InputBindingType::GameControllerAxis ) continue;
        if( binding.device_id != -1 && binding.device_id != ev.caxis.id ) continue;

        if( binding.gc_axis == static_cast<GameController::Axis>(ev.caxis.axis) ) {
          real32 magnitude = axis_magnitude(ev.caxis.value, binding.axis_direction);
          bool active = (magnitude >= binding.axis_threshold);
          real32 clamped_value = active ? magnitude : 0.0f;
          this->apply_binding_state(player_index, action_name, binding, active, clamped_value);
        }
      }
    }
  }
}

void InputActionProfileManager::process_joystick_button_event(const Event& ev)
{
  bool pressed = (ev.type == Event::JOYSTICK_BUTTON_PRESS);

  for( auto pp_it = this->player_profiles_.begin();
       pp_it != this->player_profiles_.end(); ++pp_it ) {
    int player_index = pp_it->first;
    const std::string& profile_name = pp_it->second;

    JoystickID assigned_device = this->player_device(player_index);
    if( assigned_device != -1 && assigned_device != ev.jbutton.id ) {
      continue;
    }

    auto prof = this->profile(profile_name);
    if( prof == nullptr ) continue;

    this->ensure_player_state(player_index);

    std::vector<std::string> action_names = prof->action_names();
    for( auto a_it = action_names.begin(); a_it != action_names.end(); ++a_it ) {
      const std::string& action_name = *a_it;
      const InputActionProfile::BindingList& bindings = prof->bindings(action_name);

      for( auto b_it = bindings.begin(); b_it != bindings.end(); ++b_it ) {
        const InputActionBinding& binding = *b_it;
        if( binding.type != InputBindingType::JoystickButton ) continue;
        if( binding.device_id != -1 && binding.device_id != ev.jbutton.id ) continue;

        if( binding.js_button == ev.jbutton.button ) {
          this->apply_binding_state(player_index, action_name, binding, pressed, pressed ? 1.0f : 0.0f);
        }
      }
    }
  }
}

void InputActionProfileManager::process_joystick_axis_event(const Event& ev)
{
  for( auto pp_it = this->player_profiles_.begin();
       pp_it != this->player_profiles_.end(); ++pp_it ) {
    int player_index = pp_it->first;
    const std::string& profile_name = pp_it->second;

    JoystickID assigned_device = this->player_device(player_index);
    if( assigned_device != -1 && assigned_device != ev.jaxis.id ) {
      continue;
    }

    auto prof = this->profile(profile_name);
    if( prof == nullptr ) continue;

    this->ensure_player_state(player_index);

    std::vector<std::string> action_names = prof->action_names();
    for( auto a_it = action_names.begin(); a_it != action_names.end(); ++a_it ) {
      const std::string& action_name = *a_it;
      const InputActionProfile::BindingList& bindings = prof->bindings(action_name);

      for( auto b_it = bindings.begin(); b_it != bindings.end(); ++b_it ) {
        const InputActionBinding& binding = *b_it;
        if( binding.type != InputBindingType::JoystickAxis ) continue;
        if( binding.device_id != -1 && binding.device_id != ev.jaxis.id ) continue;

        if( binding.js_axis == ev.jaxis.axis ) {
          real32 magnitude = axis_magnitude(ev.jaxis.value, binding.axis_direction);
          bool active = (magnitude >= binding.axis_threshold);
          real32 clamped_value = active ? magnitude : 0.0f;
          this->apply_binding_state(player_index, action_name, binding, active, clamped_value);
        }
      }
    }
  }
}

void InputActionProfileManager::process_joystick_hat_event(const Event& ev)
{
  for( auto pp_it = this->player_profiles_.begin();
       pp_it != this->player_profiles_.end(); ++pp_it ) {
    int player_index = pp_it->first;
    const std::string& profile_name = pp_it->second;

    JoystickID assigned_device = this->player_device(player_index);
    if( assigned_device != -1 && assigned_device != ev.jhat.id ) {
      continue;
    }

    auto prof = this->profile(profile_name);
    if( prof == nullptr ) continue;

    this->ensure_player_state(player_index);

    std::vector<std::string> action_names = prof->action_names();
    for( auto a_it = action_names.begin(); a_it != action_names.end(); ++a_it ) {
      const std::string& action_name = *a_it;
      const InputActionProfile::BindingList& bindings = prof->bindings(action_name);

      for( auto b_it = bindings.begin(); b_it != bindings.end(); ++b_it ) {
        const InputActionBinding& binding = *b_it;
        if( binding.type != InputBindingType::JoystickHat ) continue;
        if( binding.device_id != -1 && binding.device_id != ev.jhat.id ) continue;

        if( binding.js_hat == ev.jhat.hat ) {
          bool pressed = false;
          if( binding.js_hat_value != Joystick::HAT_CENTERED ) {
            pressed = (ev.jhat.value & binding.js_hat_value) != 0;
          } else {
            pressed = (ev.jhat.value == Joystick::HAT_CENTERED);
          }
          this->apply_binding_state(player_index, action_name, binding, pressed, pressed ? 1.0f : 0.0f);
        }
      }
    }
  }
}

void InputActionProfileManager::apply_binding_state(
    int player_index, const std::string& action,
    const InputActionBinding& binding,
    bool pressed, real32 value)
{
  (void)binding;

  auto ps_it = this->player_states_.find(player_index);
  if( ps_it == this->player_states_.end() ) {
    this->ensure_player_state(player_index);
    ps_it = this->player_states_.find(player_index);
    if( ps_it == this->player_states_.end() ) {
      return;
    }
  }

  ActionStateMap& states = ps_it->second;
  auto s_it = states.find(action);
  if( s_it == states.end() ) {
    states[action] = InputActionState();
    s_it = states.find(action);
  }

  InputActionState& state = s_it->second;

  if( pressed && !state.held ) {
    state.pressed = true;
  }
  if( !pressed && state.held ) {
    state.released = true;
  }
  state.held = pressed;
  if( value > state.value ) {
    state.value = value;
  }
}

bool InputActionProfileManager::is_pressed(int player_index,
                                           const std::string& action) const
{
  auto ps_it = this->player_states_.find(player_index);
  if( ps_it == this->player_states_.end() ) return false;

  auto s_it = ps_it->second.find(action);
  if( s_it == ps_it->second.end() ) return false;

  return s_it->second.pressed;
}

bool InputActionProfileManager::is_released(int player_index,
                                            const std::string& action) const
{
  auto ps_it = this->player_states_.find(player_index);
  if( ps_it == this->player_states_.end() ) return false;

  auto s_it = ps_it->second.find(action);
  if( s_it == ps_it->second.end() ) return false;

  return s_it->second.released;
}

bool InputActionProfileManager::is_held(int player_index,
                                        const std::string& action) const
{
  auto ps_it = this->player_states_.find(player_index);
  if( ps_it == this->player_states_.end() ) return false;

  auto s_it = ps_it->second.find(action);
  if( s_it == ps_it->second.end() ) return false;

  return s_it->second.held;
}

real32 InputActionProfileManager::action_value(int player_index,
                                               const std::string& action) const
{
  auto ps_it = this->player_states_.find(player_index);
  if( ps_it == this->player_states_.end() ) return 0.0f;

  auto s_it = ps_it->second.find(action);
  if( s_it == ps_it->second.end() ) return 0.0f;

  return s_it->second.value;
}

const InputActionState* InputActionProfileManager::action_state(
    int player_index, const std::string& action) const
{
  auto ps_it = this->player_states_.find(player_index);
  if( ps_it == this->player_states_.end() ) return nullptr;

  auto s_it = ps_it->second.find(action);
  if( s_it == ps_it->second.end() ) return nullptr;

  return &s_it->second;
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
