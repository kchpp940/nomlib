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
#ifndef NOMLIB_SYSTEM_INPUT_MAPPER_INPUT_ACTION_PROFILE_MANAGER_HPP
#define NOMLIB_SYSTEM_INPUT_MAPPER_INPUT_ACTION_PROFILE_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "nomlib/config.hpp"
#include "nomlib/system/Event.hpp"
#include "nomlib/system/InputMapper/InputActionProfile.hpp"
#include "nomlib/system/InputMapper/InputStateMapper.hpp"

namespace nom {

class Value;

class InputActionProfileManager
{
  public:
    typedef InputActionProfileManager SelfType;
    typedef std::map<std::string, InputActionRuntimeState> ActionStateMap;
    typedef std::map<int, ActionStateMap> PlayerStateMap;
    typedef std::map<int, std::string> PlayerProfileMap;
    typedef std::map<int, JoystickID> PlayerDeviceMap;
    typedef std::map<std::string, std::shared_ptr<InputActionProfile>> ProfileMap;
    typedef std::map<int, std::string> PlayerStateNameMap;
    typedef std::map<std::string, std::vector<InputActionBindingContribution>> ActionContributionMap;
    typedef std::map<int, ActionContributionMap> PlayerContributionMap;

    static const int DEFAULT_PLAYER = 0;

    InputActionProfileManager();

    ~InputActionProfileManager();

    bool load_profile(const std::string& name, const Value& root);

    bool add_profile(const std::string& name, std::shared_ptr<InputActionProfile> profile);

    void remove_profile(const std::string& name);

    bool has_profile(const std::string& name) const;

    std::shared_ptr<InputActionProfile> profile(const std::string& name) const;

    std::vector<std::string> profile_names() const;

    bool set_profile(int player_index, const std::string& profile_name);

    std::string active_profile(int player_index) const;

    bool set_player_device(int player_index, JoystickID device_id);

    JoystickID player_device(int player_index) const;

    std::vector<std::pair<int, JoystickID>> find_device_conflicts() const;

    void set_state_mapper(InputStateMapper* mapper);

    InputStateMapper* state_mapper() const;

    void update();

    bool is_pressed(int player_index, const std::string& action) const;

    bool is_released(int player_index, const std::string& action) const;

    bool is_held(int player_index, const std::string& action) const;

    real32 action_value(int player_index, const std::string& action) const;

    const InputActionRuntimeState* action_state(int player_index, const std::string& action) const;

    bool is_pressed(const std::string& action) const;

    bool is_released(const std::string& action) const;

    bool is_held(const std::string& action) const;

    real32 action_value(const std::string& action) const;

    void clear_states();

  private:
    static std::string player_state_name(int player_index);

    void rebuild_player_state(int player_index);

    void ensure_player_state(int player_index);

    void aggregate_action_state(int player_index, const std::string& action_name);

    ProfileMap profiles_;
    PlayerProfileMap player_profiles_;
    PlayerDeviceMap player_devices_;
    PlayerStateMap player_states_;
    PlayerContributionMap player_contributions_;
    PlayerStateNameMap player_state_names_;

    InputStateMapper* state_mapper_;
};

std::unique_ptr<InputActionProfileManager> make_unique_input_action_profile_manager();
std::shared_ptr<InputActionProfileManager> make_shared_input_action_profile_manager();

} // namespace nom

#endif // include guard defined
