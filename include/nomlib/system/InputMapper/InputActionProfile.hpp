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
#ifndef NOMLIB_SYSTEM_INPUT_MAPPER_INPUT_ACTION_PROFILE_HPP
#define NOMLIB_SYSTEM_INPUT_MAPPER_INPUT_ACTION_PROFILE_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/Event.hpp"
#include "nomlib/system/Joystick.hpp"
#include "nomlib/system/GameController.hpp"

namespace nom {

class Value;
class InputAction;

enum class InputBindingType: uint8
{
  Invalid = 0,
  Keyboard,
  GameControllerButton,
  GameControllerAxis,
  JoystickButton,
  JoystickAxis,
  JoystickHat,
};

enum class AxisDirection: uint8
{
  Positive = 0,
  Negative,
};

struct InputActionBinding
{
  InputBindingType type = InputBindingType::Invalid;

  int32 key_sym = 0;
  uint16 key_mod = 0;

  GameController::Button gc_button = GameController::BUTTON_INVALID;
  GameController::Axis gc_axis = GameController::AXIS_INVALID;
  real32 axis_threshold = 0.0f;
  AxisDirection axis_direction = AxisDirection::Positive;

  uint8 js_button = 0;
  uint8 js_axis = 0;
  uint8 js_hat = 0;
  uint8 js_hat_value = 0;

  JoystickID device_id = -1;
  bool conflict_allow = true;
};

struct InputActionRuntimeState
{
  bool pressed = false;
  bool released = false;
  bool held = false;
  real32 value = 0.0f;
  real32 prev_value = 0.0f;
  bool conflict_blocked = false;
};

class InputActionProfile
{
  public:
    typedef InputActionProfile SelfType;
    typedef std::vector<InputActionBinding> BindingList;
    typedef std::map<std::string, BindingList> ActionMap;
    typedef std::vector<std::shared_ptr<InputAction>> InputActionPtrList;

    InputActionProfile();

    ~InputActionProfile();

    const std::string& name() const;

    void set_name(const std::string& name);

    bool load_from_value(const Value& root);

    void add_binding(const std::string& action, const InputActionBinding& binding);

    const BindingList& bindings(const std::string& action) const;

    std::vector<std::string> action_names() const;

    bool has_action(const std::string& action) const;

    void clear();

    bool validate() const;

    bool validate_binding(const InputActionBinding& binding) const;

    std::vector<std::string> find_duplicate_bindings() const;

    bool bindings_equal(const InputActionBinding& a,
                        const InputActionBinding& b) const;

    InputActionPtrList create_input_actions(
        const std::string& action,
        JoystickID resolved_device_id = -1) const;

    static int32 key_name_to_sym(const std::string& name);

    static std::string key_sym_to_name(int32 sym);

    static GameController::Button gc_button_name_to_enum(const std::string& name);

    static std::string gc_button_enum_to_name(GameController::Button button);

    static GameController::Axis gc_axis_name_to_enum(const std::string& name);

    static std::string gc_axis_enum_to_name(GameController::Axis axis);

    static uint8 joystick_hat_name_to_value(const std::string& name);

    static std::string joystick_hat_value_to_name(uint8 value);

  private:
    bool parse_keyboard_bindings(const Value& node, BindingList& out);
    bool parse_game_controller_bindings(const Value& node, BindingList& out);
    bool parse_joystick_button_bindings(const Value& node, BindingList& out);
    bool parse_joystick_axis_bindings(const Value& node, BindingList& out);
    bool parse_joystick_hat_bindings(const Value& node, BindingList& out);

    std::shared_ptr<InputAction> create_keyboard_action(
        const InputActionBinding& binding, InputState state) const;

    std::shared_ptr<InputAction> create_gc_button_action(
        const InputActionBinding& binding, JoystickID device_id,
        InputState state) const;

    std::shared_ptr<InputAction> create_gc_axis_action(
        const InputActionBinding& binding, JoystickID device_id) const;

    std::shared_ptr<InputAction> create_js_button_action(
        const InputActionBinding& binding, JoystickID device_id,
        InputState state) const;

    std::shared_ptr<InputAction> create_js_axis_action(
        const InputActionBinding& binding, JoystickID device_id) const;

    std::shared_ptr<InputAction> create_js_hat_action(
        const InputActionBinding& binding, JoystickID device_id) const;

    std::string name_;
    ActionMap actions_;
    static const BindingList empty_bindings_;
};

std::unique_ptr<InputActionProfile> make_unique_input_action_profile();
std::shared_ptr<InputActionProfile> make_shared_input_action_profile();

} // namespace nom

#endif // include guard defined
