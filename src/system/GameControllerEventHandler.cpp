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

void GameControllerEventHandler::remove_joysticks() {
  auto res = this->joysticks_.begin();
  if(res == this->joysticks_.end()) {
    return;
  }

  if(res != this->joysticks_.end()) {
    // Success -- found device; say buh-bye!
    res->second->close();
    // FIXME(jeff): How do we properly erase
    this->joysticks_.clear();
  }
}

bool GameControllerEventHandler::remap_joystick(JoystickID dev_id)
{
  auto res = this->joysticks_.find(dev_id);
  if(res == this->joysticks_.end()) {
    NOM_LOG_WARN(NOM_LOG_CATEGORY_EVENT,
                 "Cannot remap game controller", dev_id,
                 ": device not found in pool");
    return false;
  }

  std::unique_ptr<GameController>& existing = res->second;
  if(existing == nullptr) {
    NOM_LOG_WARN(NOM_LOG_CATEGORY_EVENT,
                 "Cannot remap game controller", dev_id,
                 ": device pointer is null");
    return false;
  }

  JoystickIndex device_index = -1;
  const int num_joysticks = SDL_NumJoysticks();
  for(int i = 0; i < num_joysticks; ++i) {
    Joystick joy;
    if(joy.open(i) == true) {
      if(joy.device_id() == dev_id) {
        device_index = i;
        break;
      }
    }
  }

  if(device_index < 0) {
    NOM_LOG_WARN(NOM_LOG_CATEGORY_EVENT,
                 "Cannot remap game controller", dev_id,
                 ": device index not re-discoverable");
    return false;
  }

  existing->close();
  existing.reset();

  auto new_dev = nom::make_unique_game_controller();
  if(new_dev != nullptr && new_dev->open(device_index) == true) {
    JoystickID new_id = new_dev->device_id();
    if(new_id == dev_id) {
      existing = std::move(new_dev);
      NOM_LOG_INFO(NOM_LOG_CATEGORY_EVENT,
                   "Game controller", dev_id,
                   "mapping refreshed successfully");
      return true;
    } else {
      NOM_LOG_WARN(NOM_LOG_CATEGORY_EVENT,
                   "Game controller remap changed instance ID",
                   dev_id, "→", new_id);
      this->joysticks_.erase(res);
      this->joysticks_[new_id] = std::move(new_dev);
      return true;
    }
  }

  NOM_LOG_ERR(NOM_LOG_CATEGORY_EVENT,
              "Failed to re-open game controller", dev_id,
              "after remap event");
  this->joysticks_.erase(res);
  return false;
}

} // namespace nom
