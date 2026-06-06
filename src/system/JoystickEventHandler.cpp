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
#include "nomlib/system/JoystickEventHandler.hpp"

// Private headers
#include "nomlib/core/err.hpp"

namespace nom {

JoystickEventHandler::JoystickEventHandler()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_EVENT,
                      NOM_LOG_PRIORITY_VERBOSE );
}

JoystickEventHandler::~JoystickEventHandler()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_EVENT,
                      NOM_LOG_PRIORITY_VERBOSE );

  this->remove_joysticks();
}

nom::size_type JoystickEventHandler::num_joysticks() const
{
  auto result = this->joysticks_.size();

  return result;
}

Joystick* JoystickEventHandler::joystick(JoystickID dev_id) const
{
  auto res = this->joysticks_.find(dev_id);
  if( res != this->joysticks_.end() ) {
    // Success -- device found
    return res->second.get();
  } else {
    // Err -- device **not** found
    return nullptr;
  }
}

bool JoystickEventHandler::joystick_exists(JoystickID dev_id) const
{
  auto res = this->joysticks_.find(dev_id);
  if( res != this->joysticks_.end() ) {
    // Success -- device exists!
    return true;
  } else {
    // Failure -- device does **not** exist!
    return false;
  }
}

Joystick* JoystickEventHandler::add_joystick(JoystickIndex device_index)
{
  Joystick* result = nullptr;

  auto joy_dev = nom::make_unique_joystick();
  if( joy_dev != nullptr && joy_dev->open(device_index) == true ) {

    JoystickID dev_id = joy_dev->device_id();
    std::string dev_name = joy_dev->name();

    if( dev_id < 0 ) {
      // Err
      return result;
    }

    // Success!
    this->joysticks_[dev_id] = std::move(joy_dev);
    result = this->joysticks_[dev_id].get();
  }

  return result;
}

bool JoystickEventHandler::remove_joystick(JoystickID dev_id)
{
  auto res = this->joysticks_.find(dev_id);
  if( res != this->joysticks_.end() ) {

    // Success -- found device; freeing and removing
    res->second->close();
    this->joysticks_.erase(res);

    return true;
  } else {
    // Err -- device not found
    return false;
  }
}

void JoystickEventHandler::remove_joysticks()
{
  for( auto itr = this->joysticks_.begin(); itr != this->joysticks_.end(); ++itr ) {
    if( itr->second != nullptr ) {
      itr->second->close();
    }
  }
  this->joysticks_.clear();
}

// --- IJoystickEventHandler overrides ---

bool JoystickEventHandler::add_device(JoystickIndex device_index)
{
  return( this->add_joystick(device_index) != nullptr );
}

bool JoystickEventHandler::remove_device(JoystickID dev_id)
{
  return this->remove_joystick(dev_id);
}

void JoystickEventHandler::remove_all_devices()
{
  this->remove_joysticks();
}

bool JoystickEventHandler::remap_device(JoystickID dev_id)
{
  // Raw joysticks have no SDL-level mapping concept; this is a no-op.
  (void)dev_id;
  return false;
}

bool JoystickEventHandler::device_info( JoystickID dev_id,
                                        std::string* out_name,
                                        JoystickID* out_instance_id ) const
{
  auto itr = this->joysticks_.find(dev_id);
  if( itr == this->joysticks_.end() || itr->second == nullptr ) {
    return false;
  }

  if( out_name != nullptr ) {
    *out_name = itr->second->name();
  }
  if( out_instance_id != nullptr ) {
    *out_instance_id = itr->second->device_id();
  }
  return true;
}

} // namespace nom
