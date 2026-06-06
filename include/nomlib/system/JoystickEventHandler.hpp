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
#ifndef NOMLIB_SYSTEM_JOYSTICK_EVENT_HANDLER_HPP
#define NOMLIB_SYSTEM_JOYSTICK_EVENT_HANDLER_HPP

#include <memory>
#include <map>

#include "nomlib/config.hpp"
#include "nomlib/system/Joystick.hpp"
#include "nomlib/system/IJoystickEventHandler.hpp"

namespace nom {

// Forward declarations
class Joystick;

/// \brief Internal management of hot-pluggable joystick devices handling
class JoystickEventHandler
  : public IJoystickEventHandler
{
  public:
    JoystickEventHandler();
    ~JoystickEventHandler();

    /// \brief Get the number of accessible joysticks.
    nom::size_type num_joysticks() const override;

    // Non-owned pointer
    Joystick* joystick(JoystickID dev_id) const;

    /// \brief Query for the existence of an attached joystick.
    ///
    /// \returns Boolean TRUE when the joystick exists, and boolean FALSE when
    /// the joystick does **not** exist.
    bool joystick_exists(JoystickID dev_id) const override;

    /// \brief Append a joystick to the active devices pool.
    ///
    /// \returns A non-owned pointer to the joystick device on success,
    /// or NULL on failure, such as when the device can not be opened.
    Joystick* add_joystick(JoystickIndex device_index);

    /// \brief Remove a connected joystick from the joystick event pool.
    ///
    /// \returns Boolean TRUE when the joystick exists, and boolean FALSE when
    /// the joystick does **not** exist.
    bool remove_joystick(JoystickID dev_id);

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
    typedef std::map<JoystickID, std::unique_ptr<Joystick>> joysticks;

    /// \brief Active joysticks event pool.
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
