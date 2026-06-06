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
#ifndef NOMLIB_SYSTEM_GAME_CONTROLLER_EVENT_HANDLER_HPP
#define NOMLIB_SYSTEM_GAME_CONTROLLER_EVENT_HANDLER_HPP

#include <memory>
#include <map>

#include "nomlib/config.hpp"
#include "nomlib/system/GameController.hpp"
#include "nomlib/system/IJoystickEventHandler.hpp"

namespace nom {

// Forward declarations
class GameController;

/// \brief Internal management of hot-pluggable game controller devices handling
class GameControllerEventHandler
  : public IJoystickEventHandler
{
  public:
    GameControllerEventHandler();
    ~GameControllerEventHandler();

    /// \brief Get the number of accessible joysticks.
    nom::size_type num_joysticks() const override;

    // Non-owned pointer
    GameController* joystick(JoystickID dev_id) const;

    /// \brief Query for the existence of an attached joystick.
    ///
    /// \returns Boolean TRUE when the joystick exists, and boolean FALSE when
    /// the joystick does **not** exist.
    bool joystick_exists(JoystickID dev_id) const override;

    /// \brief Append a game controller to the active devices pool.
    ///
    /// \returns A non-owned pointer to the game controller device on success,
    /// or NULL on failure, such as when the device can not be opened.
    GameController* add_joystick(JoystickIndex device_index);

    /// \brief Remove a connected joystick from the joystick event pool.
    ///
    /// \returns Boolean TRUE when the joystick exists, and boolean FALSE when
    /// the joystick does **not** exist.
    bool remove_joystick(JoystickID dev_id);

    /// \brief Re-open a game controller in response to a mapping update.
    ///
    /// The controller is looked up by its current instance ID. To find the
    /// up-to-date device index reliably (SDL's device indices can be reassigned
    /// after hot-plug events), the method scans SDL_NumJoysticks() and matches
    /// against SDL_JoystickGetDeviceInstanceID(). If a matching device is
    /// found, the old controller is closed cleanly and re-opened so the new
    /// button/axis mapping from SDL takes effect; if the re-opened controller
    /// receives a different instance ID, the internal pool entry is re-keyed
    /// accordingly. If the instance ID is no longer present, the stale entry
    /// is simply removed.
    ///
    /// \returns A non-owned pointer to the re-opened game controller on
    /// success, or NULL on failure.
    GameController* remap_joystick(JoystickID dev_id);

    /// \brief Remove all joystick connection IDs from the joystick event pool.
    ///
    /// \returns void
    void remove_joysticks();

    // --- IJoystickEventHandler overrides ---

    bool add_device(JoystickIndex device_index) override;
    bool remove_device(JoystickID dev_id) override;
    void remove_all_devices() override;
    bool remap_device(JoystickID dev_id) override;
    bool device_info( JoystickID dev_id,
                      std::string* out_name,
                      JoystickID* out_instance_id ) const override;

  private:
    typedef std::map<JoystickID, std::unique_ptr<GameController>> joysticks;

    /// \brief Active game controllers event pool.
    ///
    /// \remarks This is a mapping of the current joysticks that are available
    /// to the end-user for use. Without this mapping, the end-user would be
    /// responsible for the management of each joystick's life-time; we must
    /// hold onto the device reference at the time of the SDL event, else we
    /// aren't able to use it beyond that frame -- the same is true of removal
    /// events.
    joysticks joysticks_;
};

} // namespace nom

#endif // include guard defined
