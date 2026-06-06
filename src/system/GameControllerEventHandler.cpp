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
#include "nomlib/system/GameControllerEventHandler.hpp"

// Private headers
#include "nomlib/core/err.hpp"

namespace nom {

GameControllerEventHandler::GameControllerEventHandler()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_EVENT,
                      NOM_LOG_PRIORITY_VERBOSE );
}

GameControllerEventHandler::~GameControllerEventHandler()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_EVENT,
                      NOM_LOG_PRIORITY_VERBOSE );

  this->remove_joysticks();
}

nom::size_type GameControllerEventHandler::num_joysticks() const
{
  auto result = this->joysticks_.size();
  return result;
}

GameController* GameControllerEventHandler::joystick(JoystickID dev_id) const
{
  GameController* result = nullptr;

  auto res = this->joysticks_.find(dev_id);
  if(res == this->joysticks_.end()) {
    return result;
  }

  if( res != this->joysticks_.end() ) {
    // Success -- device found
    result = res->second.get();
  }

  return result;
}

// TODO(jeff): Use GameControllerEventHandler::joystick method within this
// function!
bool GameControllerEventHandler::joystick_exists(JoystickID dev_id) const {
  bool result = false;
  auto res = this->joysticks_.find(dev_id);
  if(res == this->joysticks_.end()) {
    return result;
  }

  if( res != this->joysticks_.end() ) {
    // Success -- device exists
    result = true;
  }

  return result;
}

GameController*
GameControllerEventHandler::add_joystick(JoystickIndex device_index)
{
  GameController* result = nullptr;

  auto joy_dev = nom::make_unique_game_controller();
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

bool GameControllerEventHandler::remove_joystick(JoystickID dev_id)
{
  bool result = false;

  auto res = this->joysticks_.find(dev_id);
  if(res == this->joysticks_.end()) {
    return result;
  }

  if( res != this->joysticks_.end() ) {

    // Success -- found device; say buh-bye!
    res->second->close();
    this->joysticks_.erase(res);
    result = true;
  }

  return result;
}

GameController*
GameControllerEventHandler::remap_joystick(JoystickID dev_id)
{
  GameController* result = nullptr;

  auto res = this->joysticks_.find(dev_id);
  if( res == this->joysticks_.end() ) {
    nom::set_error("Game controller instance ID not found");
    return result;
  }

  JoystickIndex device_index = -1;
  const int num_joysticks = SDL_NumJoysticks();
  for( int i = 0; i < num_joysticks; ++i ) {
    if( SDL_IsGameController(i) != SDL_TRUE ) {
      continue;
    }
    const JoystickID inst_id = SDL_JoystickGetDeviceInstanceID(i);
    if( inst_id == dev_id ) {
      device_index = i;
      break;
    }
  }

  res->second->close();
  this->joysticks_.erase(res);

  if( device_index < 0 ) {
    NOM_LOG_WARN( NOM_LOG_CATEGORY_EVENT,
                  "Game controller instance ID", dev_id,
                  "no longer present in device list; removing stale entry" );
    nom::set_error("Game controller not present for remap");
    return result;
  }

  result = this->add_joystick(device_index);
  if( result != nullptr ) {
    JoystickID new_dev_id = result->device_id();
    NOM_LOG_INFO( NOM_LOG_CATEGORY_EVENT,
                  "Re-mapped game controller", result->name(),
                  "old instance ID:", dev_id,
                  "new instance ID:", new_dev_id );
  } else {
    NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                  "Failed to re-open game controller after remap:",
                  nom::error() );
  }

  return result;
}

void GameControllerEventHandler::remove_joysticks() {
  for( auto itr = this->joysticks_.begin(); itr != this->joysticks_.end(); ++itr ) {
    if( itr->second != nullptr ) {
      itr->second->close();
    }
  }
  this->joysticks_.clear();
}

// --- IJoystickEventHandler overrides ---

bool GameControllerEventHandler::add_device(JoystickIndex device_index)
{
  return( this->add_joystick(device_index) != nullptr );
}

bool GameControllerEventHandler::remove_device(JoystickID dev_id)
{
  return this->remove_joystick(dev_id);
}

void GameControllerEventHandler::remove_all_devices()
{
  this->remove_joysticks();
}

bool GameControllerEventHandler::remap_device(JoystickID dev_id)
{
  return( this->remap_joystick(dev_id) != nullptr );
}

bool GameControllerEventHandler::device_info( JoystickID dev_id,
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

bool GameControllerEventHandler::on_device_added( JoystickIndex device_index,
                                                  std::string* out_name,
                                                  JoystickID* out_instance_id )
{
  GameController* dev = this->add_joystick(device_index);
  if( dev == nullptr ) {
    return false;
  }
  return this->device_info(dev->device_id(), out_name, out_instance_id);
}

bool GameControllerEventHandler::on_device_removed(JoystickID dev_id)
{
  return this->remove_joystick(dev_id);
}

bool GameControllerEventHandler::on_device_remapped( JoystickID old_instance_id,
                                                     std::string* out_name,
                                                     JoystickID* out_new_instance_id )
{
  GameController* dev = this->remap_joystick(old_instance_id);
  if( dev == nullptr ) {
    return false;
  }
  return this->device_info(dev->device_id(), out_name, out_new_instance_id);
}

} // namespace nom
