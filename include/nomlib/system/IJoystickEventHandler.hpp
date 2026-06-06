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
#ifndef NOMLIB_SYSTEM_IJOYSTICK_EVENT_HANDLER_HPP
#define NOMLIB_SYSTEM_IJOYSTICK_EVENT_HANDLER_HPP

#include <string>

#include "nomlib/config.hpp"

namespace nom {

/// \brief Unified lifecycle interface for joystick / game controller device
/// pools.
///
/// \remarks Both JoystickEventHandler and GameControllerEventHandler implement
/// this interface so that EventHandler can manage either type through a single
/// code path for enable, disable, device add/remove/clear and (where
/// applicable) remap operations.
class IJoystickEventHandler
{
  public:
    virtual ~IJoystickEventHandler() {}

    /// \brief Get the number of accessible joysticks / controllers in the pool.
    virtual nom::size_type num_joysticks() const = 0;

    /// \brief Query whether a device with the given instance ID is present.
    virtual bool joystick_exists(JoystickID dev_id) const = 0;

    /// \brief Open a device by its SDL device index and add it to the pool.
    ///
    /// \returns Boolean TRUE on success, FALSE on failure.
    ///
    /// \remarks After a successful add, the caller can use
    /// nom::IJoystickEventHandler::device_info to retrieve the instance-level
    /// information for logging or event construction without needing to know
    /// the concrete device type.
    virtual bool add_device(JoystickIndex device_index) = 0;

    /// \brief Remove a device from the pool by its instance ID.
    ///
    /// \returns Boolean TRUE when the device was found and removed, FALSE
    /// otherwise.
    virtual bool remove_device(JoystickID dev_id) = 0;

    /// \brief Close and remove every device in the pool.
    virtual void remove_all_devices() = 0;

    /// \brief Re-open a device (when supported) after a mapping change.
    ///
    /// For device types that do not have a mapping concept (e.g. raw
    /// joysticks) this is a no-op that returns FALSE.
    ///
    /// \returns Boolean TRUE when the device was found and successfully
    /// re-opened, FALSE otherwise.
    virtual bool remap_device(JoystickID dev_id) = 0;

    /// \brief Query the display name and instance ID of a previously-added
    /// device.
    ///
    /// \param dev_id          The instance ID of the device to look up.
    /// \param out_name        If non-NULL, populated with the device name on
    ///                        success.
    /// \param out_instance_id If non-NULL, populated with the current instance
    ///                        ID of the device on success.
    ///
    /// \returns Boolean TRUE when the device exists and the look-up succeeds,
    /// FALSE otherwise.
    virtual bool device_info( JoystickID dev_id,
                              std::string* out_name,
                              JoystickID* out_instance_id ) const = 0;

    // --- High-level device event handlers (called by EventHandler) ---

    /// \brief Handle a device-added event: open the device at the given SDL
    /// device index and register it in the pool.
    ///
    /// \param device_index    The SDL device index (e.g. ev->jdevice.which or
    ///                        ev->cdevice.which for the ADDED event).
    /// \param out_name        If non-NULL, populated with the device name on
    ///                        success.
    /// \param out_instance_id If non-NULL, populated with the instance ID
    ///                        assigned by SDL on success.
    ///
    /// \returns Boolean TRUE on success, FALSE on failure.
    virtual bool on_device_added( JoystickIndex device_index,
                                  std::string* out_name,
                                  JoystickID* out_instance_id ) = 0;

    /// \brief Handle a device-removed event: close and unregister the device
    /// with the given instance ID.
    ///
    /// \returns Boolean TRUE when the device was found and removed, FALSE
    /// otherwise.
    virtual bool on_device_removed(JoystickID dev_id) = 0;

    /// \brief Handle a device-remapped event. For device types that support
    /// mapping (game controllers) this closes and re-opens the device so that
    /// the new mapping takes effect. For raw joysticks it is a no-op.
    ///
    /// \param old_instance_id  The instance ID carried by the SDL remap event.
    /// \param out_name         If non-NULL, populated with the device name
    ///                         after a successful remap.
    /// \param out_new_instance_id If non-NULL, populated with the instance ID
    ///                            after a successful remap (may differ from
    ///                            old_instance_id).
    ///
    /// \returns Boolean TRUE when the remap succeeds, FALSE otherwise.
    virtual bool on_device_remapped( JoystickID old_instance_id,
                                     std::string* out_name,
                                     JoystickID* out_new_instance_id ) = 0;
};

} // namespace nom

#endif // include guard defined

/// \class nom::IJoystickEventHandler
/// \ingroup system
///
