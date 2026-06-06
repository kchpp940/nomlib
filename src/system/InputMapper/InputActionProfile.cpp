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
#include "nomlib/system/InputMapper/InputActionProfile.hpp"

#include <algorithm>
#include <cctype>

#include "nomlib/core/err.hpp"
#include "nomlib/ptree.hpp"

#include <SDL_keycode.h>
#include <SDL_gamecontroller.h>

namespace nom {

const InputActionProfile::BindingList InputActionProfile::empty_bindings_;

namespace {

std::string to_upper(const std::string& s)
{
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  return result;
}

std::string to_lower(const std::string& s)
{
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

} // anonymous namespace

InputActionProfile::InputActionProfile()
{
}

InputActionProfile::~InputActionProfile()
{
}

const std::string& InputActionProfile::name() const
{
  return this->name_;
}

void InputActionProfile::set_name(const std::string& name)
{
  this->name_ = name;
}

bool InputActionProfile::load_from_value(const Value& root)
{
  this->clear();

  if( root.object_type() == false ) {
    NOM_LOG_ERR( NOM, "Input action profile root must be a JSON object" );
    return false;
  }

  const Value& name_val = root["name"];
  if( name_val.string_type() ) {
    this->name_ = name_val.get_string();
  }

  const Value& actions_val = root["actions"];
  if( actions_val.object_type() == false ) {
    NOM_LOG_ERR( NOM, "Input action profile must have an 'actions' object" );
    return false;
  }

  Value::Members action_names = actions_val.member_names();
  for( auto it = action_names.begin(); it != action_names.end(); ++it ) {
    const std::string& action_name = *it;
    const Value& action_node = actions_val[action_name];

    if( action_node.object_type() == false ) {
      NOM_LOG_WARN( NOM, "Action '" + action_name + "' is not an object, skipping" );
      continue;
    }

    BindingList bindings;
    bool conflict_allow = true;

    const Value& conflict_val = action_node["conflict_allow"];
    if( conflict_val.bool_type() ) {
      conflict_allow = conflict_val.get_bool();
    }

    const Value& keyboard_val = action_node["keyboard"];
    if( keyboard_val.array_type() ) {
      this->parse_keyboard_bindings(keyboard_val, bindings);
    }

    const Value& gc_val = action_node["game_controller"];
    if( gc_val.array_type() ) {
      this->parse_game_controller_bindings(gc_val, bindings);
    }

    const Value& js_btn_val = action_node["joystick_button"];
    if( js_btn_val.array_type() ) {
      this->parse_joystick_button_bindings(js_btn_val, bindings);
    }

    const Value& js_axis_val = action_node["joystick_axis"];
    if( js_axis_val.array_type() ) {
      this->parse_joystick_axis_bindings(js_axis_val, bindings);
    }

    const Value& js_hat_val = action_node["joystick_hat"];
    if( js_hat_val.array_type() ) {
      this->parse_joystick_hat_bindings(js_hat_val, bindings);
    }

    for( auto& b : bindings ) {
      b.conflict_allow = conflict_allow;
      this->add_binding(action_name, b);
    }
  }

  return true;
}

void InputActionProfile::add_binding(const std::string& action,
                                      const InputActionBinding& binding)
{
  this->actions_[action].push_back(binding);
}

const InputActionProfile::BindingList&
InputActionProfile::bindings(const std::string& action) const
{
  auto it = this->actions_.find(action);
  if( it == this->actions_.end() ) {
    return empty_bindings_;
  }
  return it->second;
}

std::vector<std::string> InputActionProfile::action_names() const
{
  std::vector<std::string> names;
  names.reserve(this->actions_.size());
  for( auto it = this->actions_.begin(); it != this->actions_.end(); ++it ) {
    names.push_back(it->first);
  }
  return names;
}

bool InputActionProfile::has_action(const std::string& action) const
{
  return this->actions_.find(action) != this->actions_.end();
}

void InputActionProfile::clear()
{
  this->name_.clear();
  this->actions_.clear();
}

bool InputActionProfile::parse_keyboard_bindings(const Value& node, BindingList& out)
{
  if( node.array_type() == false ) {
    return false;
  }

  for( nom::size_type i = 0; i < node.size(); ++i ) {
    const Value& entry = node[i];
    if( entry.object_type() == false ) continue;

    InputActionBinding binding;
    binding.type = InputBindingType::Keyboard;

    const Value& key_val = entry["key"];
    if( key_val.string_type() ) {
      binding.key_sym = key_name_to_sym(key_val.get_string());
    }

    const Value& mod_val = entry["mod"];
    if( mod_val.string_type() ) {
      std::string mod_str = to_upper(mod_val.get_string());
      if( mod_str == "SHIFT" ) binding.key_mod = KMOD_SHIFT;
      else if( mod_str == "CTRL" || mod_str == "CONTROL" ) binding.key_mod = KMOD_CTRL;
      else if( mod_str == "ALT" ) binding.key_mod = KMOD_ALT;
      else if( mod_str == "GUI" || mod_str == "META" ) binding.key_mod = KMOD_GUI;
    }

    const Value& device_val = entry["device_id"];
    if( device_val.int_type() ) {
      binding.device_id = device_val.get_int();
    }

    if( binding.key_sym != 0 ) {
      out.push_back(binding);
    }
  }

  return true;
}

bool InputActionProfile::parse_game_controller_bindings(const Value& node, BindingList& out)
{
  if( node.array_type() == false ) {
    return false;
  }

  for( nom::size_type i = 0; i < node.size(); ++i ) {
    const Value& entry = node[i];
    if( entry.object_type() == false ) continue;

    const Value& button_val = entry["button"];
    const Value& axis_val = entry["axis"];

    if( button_val.string_type() ) {
      InputActionBinding binding;
      binding.type = InputBindingType::GameControllerButton;
      binding.gc_button = gc_button_name_to_enum(button_val.get_string());

      const Value& device_val = entry["device_id"];
      if( device_val.int_type() ) {
        binding.device_id = device_val.get_int();
      }

      if( binding.gc_button != GameController::BUTTON_INVALID ) {
        out.push_back(binding);
      }
    } else if( axis_val.string_type() ) {
      InputActionBinding binding;
      binding.type = InputBindingType::GameControllerAxis;
      binding.gc_axis = gc_axis_name_to_enum(axis_val.get_string());

      const Value& threshold_val = entry["threshold"];
      if( threshold_val.double_type() || threshold_val.int_type() ) {
        binding.axis_threshold = threshold_val.get_float();
      } else {
        binding.axis_threshold = 0.2f;
      }

      const Value& dir_val = entry["direction"];
      if( dir_val.string_type() ) {
        std::string dir_str = to_lower(dir_val.get_string());
        if( dir_str == "negative" ) {
          binding.axis_direction = AxisDirection::Negative;
        } else {
          binding.axis_direction = AxisDirection::Positive;
        }
      }

      const Value& device_val = entry["device_id"];
      if( device_val.int_type() ) {
        binding.device_id = device_val.get_int();
      }

      if( binding.gc_axis != GameController::AXIS_INVALID ) {
        out.push_back(binding);
      }
    }
  }

  return true;
}

bool InputActionProfile::parse_joystick_button_bindings(const Value& node, BindingList& out)
{
  if( node.array_type() == false ) {
    return false;
  }

  for( nom::size_type i = 0; i < node.size(); ++i ) {
    const Value& entry = node[i];
    if( entry.object_type() == false ) continue;

    InputActionBinding binding;
    binding.type = InputBindingType::JoystickButton;

    const Value& btn_val = entry["button"];
    if( btn_val.int_type() || btn_val.uint_type() ) {
      binding.js_button = static_cast<uint8>(btn_val.get_int());
    }

    const Value& device_val = entry["device_id"];
    if( device_val.int_type() ) {
      binding.device_id = device_val.get_int();
    }

    out.push_back(binding);
  }

  return true;
}

bool InputActionProfile::parse_joystick_axis_bindings(const Value& node, BindingList& out)
{
  if( node.array_type() == false ) {
    return false;
  }

  for( nom::size_type i = 0; i < node.size(); ++i ) {
    const Value& entry = node[i];
    if( entry.object_type() == false ) continue;

    InputActionBinding binding;
    binding.type = InputBindingType::JoystickAxis;

    const Value& axis_val = entry["axis"];
    if( axis_val.int_type() || axis_val.uint_type() ) {
      binding.js_axis = static_cast<uint8>(axis_val.get_int());
    }

    const Value& threshold_val = entry["threshold"];
    if( threshold_val.double_type() || threshold_val.int_type() ) {
      binding.axis_threshold = threshold_val.get_float();
    } else {
      binding.axis_threshold = 0.2f;
    }

    const Value& dir_val = entry["direction"];
    if( dir_val.string_type() ) {
      std::string dir_str = to_lower(dir_val.get_string());
      if( dir_str == "negative" ) {
        binding.axis_direction = AxisDirection::Negative;
      } else {
        binding.axis_direction = AxisDirection::Positive;
      }
    }

    const Value& device_val = entry["device_id"];
    if( device_val.int_type() ) {
      binding.device_id = device_val.get_int();
    }

    out.push_back(binding);
  }

  return true;
}

bool InputActionProfile::parse_joystick_hat_bindings(const Value& node, BindingList& out)
{
  if( node.array_type() == false ) {
    return false;
  }

  for( nom::size_type i = 0; i < node.size(); ++i ) {
    const Value& entry = node[i];
    if( entry.object_type() == false ) continue;

    InputActionBinding binding;
    binding.type = InputBindingType::JoystickHat;

    const Value& hat_val = entry["hat"];
    if( hat_val.int_type() || hat_val.uint_type() ) {
      binding.js_hat = static_cast<uint8>(hat_val.get_int());
    }

    const Value& value_val = entry["value"];
    if( value_val.string_type() ) {
      binding.js_hat_value = joystick_hat_name_to_value(value_val.get_string());
    } else if( value_val.int_type() || value_val.uint_type() ) {
      binding.js_hat_value = static_cast<uint8>(value_val.get_int());
    }

    const Value& device_val = entry["device_id"];
    if( device_val.int_type() ) {
      binding.device_id = device_val.get_int();
    }

    out.push_back(binding);
  }

  return true;
}

int32 InputActionProfile::key_name_to_sym(const std::string& name)
{
  std::string upper = to_upper(name);

  if( upper.length() == 1 ) {
    char c = upper[0];
    if( c >= 'A' && c <= 'Z' ) {
      return static_cast<int32>(SDLK_a + (c - 'A'));
    }
    if( c >= '0' && c <= '9' ) {
      return static_cast<int32>(SDLK_0 + (c - '0'));
    }
  }

  if( upper == "SPACE" ) return SDLK_SPACE;
  if( upper == "RETURN" || upper == "ENTER" ) return SDLK_RETURN;
  if( upper == "ESCAPE" || upper == "ESC" ) return SDLK_ESCAPE;
  if( upper == "TAB" ) return SDLK_TAB;
  if( upper == "BACKSPACE" ) return SDLK_BACKSPACE;
  if( upper == "DELETE" || upper == "DEL" ) return SDLK_DELETE;
  if( upper == "INSERT" ) return SDLK_INSERT;
  if( upper == "HOME" ) return SDLK_HOME;
  if( upper == "END" ) return SDLK_END;
  if( upper == "PAGEUP" || upper == "PAGE_UP" ) return SDLK_PAGEUP;
  if( upper == "PAGEDOWN" || upper == "PAGE_DOWN" ) return SDLK_PAGEDOWN;

  if( upper == "LEFT" ) return SDLK_LEFT;
  if( upper == "RIGHT" ) return SDLK_RIGHT;
  if( upper == "UP" ) return SDLK_UP;
  if( upper == "DOWN" ) return SDLK_DOWN;

  if( upper == "F1" ) return SDLK_F1;
  if( upper == "F2" ) return SDLK_F2;
  if( upper == "F3" ) return SDLK_F3;
  if( upper == "F4" ) return SDLK_F4;
  if( upper == "F5" ) return SDLK_F5;
  if( upper == "F6" ) return SDLK_F6;
  if( upper == "F7" ) return SDLK_F7;
  if( upper == "F8" ) return SDLK_F8;
  if( upper == "F9" ) return SDLK_F9;
  if( upper == "F10" ) return SDLK_F10;
  if( upper == "F11" ) return SDLK_F11;
  if( upper == "F12" ) return SDLK_F12;

  if( upper == "LSHIFT" || upper == "LEFT_SHIFT" ) return SDLK_LSHIFT;
  if( upper == "RSHIFT" || upper == "RIGHT_SHIFT" ) return SDLK_RSHIFT;
  if( upper == "LCTRL" || upper == "LEFT_CTRL" ) return SDLK_LCTRL;
  if( upper == "RCTRL" || upper == "RIGHT_CTRL" ) return SDLK_RCTRL;
  if( upper == "LALT" || upper == "LEFT_ALT" ) return SDLK_LALT;
  if( upper == "RALT" || upper == "RIGHT_ALT" ) return SDLK_RALT;
  if( upper == "LGUI" || upper == "LEFT_GUI" || upper == "LEFT_META" ) return SDLK_LGUI;
  if( upper == "RGUI" || upper == "RIGHT_GUI" || upper == "RIGHT_META" ) return SDLK_RGUI;

  if( upper == "MINUS" || upper == "-" ) return SDLK_MINUS;
  if( upper == "EQUALS" || upper == "=" ) return SDLK_EQUALS;
  if( upper == "LEFTBRACKET" || upper == "[" ) return SDLK_LEFTBRACKET;
  if( upper == "RIGHTBRACKET" || upper == "]" ) return SDLK_RIGHTBRACKET;
  if( upper == "SEMICOLON" || upper == ";" ) return SDLK_SEMICOLON;
  if( upper == "APOSTROPHE" || upper == "'" ) return SDLK_QUOTE;
  if( upper == "COMMA" || upper == "," ) return SDLK_COMMA;
  if( upper == "PERIOD" || upper == "." ) return SDLK_PERIOD;
  if( upper == "SLASH" || upper == "/" ) return SDLK_SLASH;
  if( upper == "BACKSLASH" || upper == "\\" ) return SDLK_BACKSLASH;

  if( upper == "KP_0" ) return SDLK_KP_0;
  if( upper == "KP_1" ) return SDLK_KP_1;
  if( upper == "KP_2" ) return SDLK_KP_2;
  if( upper == "KP_3" ) return SDLK_KP_3;
  if( upper == "KP_4" ) return SDLK_KP_4;
  if( upper == "KP_5" ) return SDLK_KP_5;
  if( upper == "KP_6" ) return SDLK_KP_6;
  if( upper == "KP_7" ) return SDLK_KP_7;
  if( upper == "KP_8" ) return SDLK_KP_8;
  if( upper == "KP_9" ) return SDLK_KP_9;
  if( upper == "KP_DIVIDE" ) return SDLK_KP_DIVIDE;
  if( upper == "KP_MULTIPLY" ) return SDLK_KP_MULTIPLY;
  if( upper == "KP_MINUS" ) return SDLK_KP_MINUS;
  if( upper == "KP_PLUS" ) return SDLK_KP_PLUS;
  if( upper == "KP_ENTER" ) return SDLK_KP_ENTER;
  if( upper == "KP_PERIOD" ) return SDLK_KP_PERIOD;

  return 0;
}

std::string InputActionProfile::key_sym_to_name(int32 sym)
{
  switch( sym ) {
    case SDLK_SPACE: return "SPACE";
    case SDLK_RETURN: return "RETURN";
    case SDLK_ESCAPE: return "ESCAPE";
    case SDLK_TAB: return "TAB";
    case SDLK_BACKSPACE: return "BACKSPACE";
    case SDLK_DELETE: return "DELETE";
    case SDLK_INSERT: return "INSERT";
    case SDLK_HOME: return "HOME";
    case SDLK_END: return "END";
    case SDLK_PAGEUP: return "PAGEUP";
    case SDLK_PAGEDOWN: return "PAGEDOWN";
    case SDLK_LEFT: return "LEFT";
    case SDLK_RIGHT: return "RIGHT";
    case SDLK_UP: return "UP";
    case SDLK_DOWN: return "DOWN";
    case SDLK_F1: return "F1";
    case SDLK_F2: return "F2";
    case SDLK_F3: return "F3";
    case SDLK_F4: return "F4";
    case SDLK_F5: return "F5";
    case SDLK_F6: return "F6";
    case SDLK_F7: return "F7";
    case SDLK_F8: return "F8";
    case SDLK_F9: return "F9";
    case SDLK_F10: return "F10";
    case SDLK_F11: return "F11";
    case SDLK_F12: return "F12";
    case SDLK_LSHIFT: return "LSHIFT";
    case SDLK_RSHIFT: return "RSHIFT";
    case SDLK_LCTRL: return "LCTRL";
    case SDLK_RCTRL: return "RCTRL";
    case SDLK_LALT: return "LALT";
    case SDLK_RALT: return "RALT";
    case SDLK_LGUI: return "LGUI";
    case SDLK_RGUI: return "RGUI";
    default: break;
  }

  if( sym >= SDLK_a && sym <= SDLK_z ) {
    return std::string(1, static_cast<char>('A' + (sym - SDLK_a)));
  }
  if( sym >= SDLK_0 && sym <= SDLK_9 ) {
    return std::string(1, static_cast<char>('0' + (sym - SDLK_0)));
  }

  return std::string();
}

GameController::Button InputActionProfile::gc_button_name_to_enum(const std::string& name)
{
  std::string upper = to_upper(name);

  if( upper == "A" ) return GameController::BUTTON_A;
  if( upper == "B" ) return GameController::BUTTON_B;
  if( upper == "X" ) return GameController::BUTTON_X;
  if( upper == "Y" ) return GameController::BUTTON_Y;
  if( upper == "BACK" ) return GameController::BUTTON_BACK;
  if( upper == "GUIDE" ) return GameController::BUTTON_GUIDE;
  if( upper == "START" ) return GameController::BUTTON_START;
  if( upper == "LEFT_STICK" || upper == "LEFTSTICK" ) return GameController::BUTTON_LEFT_STICK;
  if( upper == "RIGHT_STICK" || upper == "RIGHTSTICK" ) return GameController::BUTTON_RIGHT_STICK;
  if( upper == "LEFT_SHOULDER" || upper == "LEFTSHOULDER" || upper == "LB" ) return GameController::BUTTON_LEFT_SHOULDER;
  if( upper == "RIGHT_SHOULDER" || upper == "RIGHTSHOULDER" || upper == "RB" ) return GameController::BUTTON_RIGHT_SHOULDER;
  if( upper == "DPAD_UP" || upper == "DPADUP" ) return GameController::BUTTON_DPAD_UP;
  if( upper == "DPAD_DOWN" || upper == "DPADDOWN" ) return GameController::BUTTON_DPAD_DOWN;
  if( upper == "DPAD_LEFT" || upper == "DPADLEFT" ) return GameController::BUTTON_DPAD_LEFT;
  if( upper == "DPAD_RIGHT" || upper == "DPADRIGHT" ) return GameController::BUTTON_DPAD_RIGHT;

  return GameController::BUTTON_INVALID;
}

std::string InputActionProfile::gc_button_enum_to_name(GameController::Button button)
{
  switch( button ) {
    case GameController::BUTTON_A: return "A";
    case GameController::BUTTON_B: return "B";
    case GameController::BUTTON_X: return "X";
    case GameController::BUTTON_Y: return "Y";
    case GameController::BUTTON_BACK: return "BACK";
    case GameController::BUTTON_GUIDE: return "GUIDE";
    case GameController::BUTTON_START: return "START";
    case GameController::BUTTON_LEFT_STICK: return "LEFT_STICK";
    case GameController::BUTTON_RIGHT_STICK: return "RIGHT_STICK";
    case GameController::BUTTON_LEFT_SHOULDER: return "LEFT_SHOULDER";
    case GameController::BUTTON_RIGHT_SHOULDER: return "RIGHT_SHOULDER";
    case GameController::BUTTON_DPAD_UP: return "DPAD_UP";
    case GameController::BUTTON_DPAD_DOWN: return "DPAD_DOWN";
    case GameController::BUTTON_DPAD_LEFT: return "DPAD_LEFT";
    case GameController::BUTTON_DPAD_RIGHT: return "DPAD_RIGHT";
    default: return std::string();
  }
}

GameController::Axis InputActionProfile::gc_axis_name_to_enum(const std::string& name)
{
  std::string upper = to_upper(name);

  if( upper == "LEFT_X" || upper == "LEFTX" ) return GameController::AXIS_LEFT_X;
  if( upper == "LEFT_Y" || upper == "LEFTY" ) return GameController::AXIS_LEFT_Y;
  if( upper == "RIGHT_X" || upper == "RIGHTX" ) return GameController::AXIS_RIGHT_X;
  if( upper == "RIGHT_Y" || upper == "RIGHTY" ) return GameController::AXIS_RIGHT_Y;
  if( upper == "TRIGGER_LEFT" || upper == "TRIGGERLEFT" || upper == "LT" ) return GameController::AXIS_TRIGGER_LEFT;
  if( upper == "TRIGGER_RIGHT" || upper == "TRIGGERRIGHT" || upper == "RT" ) return GameController::AXIS_TRIGGER_RIGHT;

  return GameController::AXIS_INVALID;
}

std::string InputActionProfile::gc_axis_enum_to_name(GameController::Axis axis)
{
  switch( axis ) {
    case GameController::AXIS_LEFT_X: return "LEFT_X";
    case GameController::AXIS_LEFT_Y: return "LEFT_Y";
    case GameController::AXIS_RIGHT_X: return "RIGHT_X";
    case GameController::AXIS_RIGHT_Y: return "RIGHT_Y";
    case GameController::AXIS_TRIGGER_LEFT: return "TRIGGER_LEFT";
    case GameController::AXIS_TRIGGER_RIGHT: return "TRIGGER_RIGHT";
    default: return std::string();
  }
}

uint8 InputActionProfile::joystick_hat_name_to_value(const std::string& name)
{
  std::string upper = to_upper(name);

  if( upper == "CENTERED" ) return Joystick::HAT_CENTERED;
  if( upper == "UP" ) return Joystick::HAT_UP;
  if( upper == "RIGHT" ) return Joystick::HAT_RIGHT;
  if( upper == "DOWN" ) return Joystick::HAT_DOWN;
  if( upper == "LEFT" ) return Joystick::HAT_LEFT;
  if( upper == "RIGHTUP" || upper == "RIGHT_UP" ) return Joystick::HAT_RIGHTUP;
  if( upper == "RIGHTDOWN" || upper == "RIGHT_DOWN" ) return Joystick::HAT_RIGHTDOWN;
  if( upper == "LEFTUP" || upper == "LEFT_UP" ) return Joystick::HAT_LEFTUP;
  if( upper == "LEFTDOWN" || upper == "LEFT_DOWN" ) return Joystick::HAT_LEFTDOWN;

  return Joystick::HAT_CENTERED;
}

std::string InputActionProfile::joystick_hat_value_to_name(uint8 value)
{
  switch( value ) {
    case Joystick::HAT_CENTERED: return "CENTERED";
    case Joystick::HAT_UP: return "UP";
    case Joystick::HAT_RIGHT: return "RIGHT";
    case Joystick::HAT_DOWN: return "DOWN";
    case Joystick::HAT_LEFT: return "LEFT";
    case Joystick::HAT_RIGHTUP: return "RIGHTUP";
    case Joystick::HAT_RIGHTDOWN: return "RIGHTDOWN";
    case Joystick::HAT_LEFTUP: return "LEFTUP";
    case Joystick::HAT_LEFTDOWN: return "LEFTDOWN";
    default: return std::string();
  }
}

std::unique_ptr<InputActionProfile> make_unique_input_action_profile()
{
  return std::unique_ptr<InputActionProfile>(new InputActionProfile());
}

std::shared_ptr<InputActionProfile> make_shared_input_action_profile()
{
  return std::shared_ptr<InputActionProfile>(new InputActionProfile());
}

} // namespace nom
