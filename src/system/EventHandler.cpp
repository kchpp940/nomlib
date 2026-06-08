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
#include "nomlib/system/EventHandler.hpp"

// Private headers
#include "nomlib/core/err.hpp"
#include "nomlib/core/clock.hpp"
#include "nomlib/core/strings.hpp"
#include "nomlib/core/unique_ptr.hpp"
#include "nomlib/system/SDL_helpers.hpp"
#include "nomlib/system/JoystickEventHandler.hpp"
#include "nomlib/system/GameControllerEventHandler.hpp"

#include <SDL.h>

static_assert(  nom::InputState::RELEASED ==
                SDL_RELEASED, "Event mismatch" );
static_assert(  nom::InputState::PRESSED ==
                SDL_PRESSED, "Event mismatch" );

static_assert(  nom::MouseButton::LEFT_MOUSE_BUTTON ==
                SDL_BUTTON_LEFT, "Event mismatch" );
static_assert(  nom::MouseButton::MIDDLE_MOUSE_BUTTON ==
                SDL_BUTTON_MIDDLE, "Event mismatch" );
static_assert(  nom::MouseButton::RIGHT_MOUSE_BUTTON ==
                SDL_BUTTON_RIGHT, "Event mismatch" );
static_assert(  nom::MouseButton::X1_MOUSE_BUTTON ==
                SDL_BUTTON_X1, "Event mismatch" );
static_assert(  nom::MouseButton::X2_MOUSE_BUTTON ==
                SDL_BUTTON_X2, "Event mismatch" );

namespace nom {

// ---------------------------------------------------------------------------
// event_watcher
// ---------------------------------------------------------------------------

struct event_watcher
{
  event_filter callback = nullptr;
  void* data1 = nullptr;
};

// ---------------------------------------------------------------------------
// EventConverter — platform event to nom::Event translation
// ---------------------------------------------------------------------------

bool EventHandler::EventConverter::convert_quit(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_QUIT ) return false;
  out.type = Event::QUIT_EVENT;
  out.timestamp = ev->quit.timestamp;
  out.quit.data1 = nullptr;
  out.quit.data2 = nullptr;
  return true;
}

namespace {

WindowEvent::EventType window_event_type(uint8 sdl_window_event)
{
  switch( sdl_window_event )
  {
    default:                        return WindowEvent::NONE;
    case SDL_WINDOWEVENT_SHOWN:     return WindowEvent::SHOWN;
    case SDL_WINDOWEVENT_HIDDEN:    return WindowEvent::HIDDEN;
    case SDL_WINDOWEVENT_EXPOSED:   return WindowEvent::EXPOSED;
    case SDL_WINDOWEVENT_MOVED:     return WindowEvent::MOVED;
    case SDL_WINDOWEVENT_RESIZED:   return WindowEvent::RESIZED;
    case SDL_WINDOWEVENT_SIZE_CHANGED: return WindowEvent::SIZE_CHANGED;
    case SDL_WINDOWEVENT_MINIMIZED: return WindowEvent::MINIMIZED;
    case SDL_WINDOWEVENT_MAXIMIZED: return WindowEvent::MAXIMIZED;
    case SDL_WINDOWEVENT_RESTORED:  return WindowEvent::RESTORED;
    case SDL_WINDOWEVENT_ENTER:     return WindowEvent::MOUSE_FOCUS_GAINED;
    case SDL_WINDOWEVENT_LEAVE:     return WindowEvent::MOUSE_FOCUS_LOST;
    case SDL_WINDOWEVENT_FOCUS_GAINED: return WindowEvent::KEYBOARD_FOCUS_GAINED;
    case SDL_WINDOWEVENT_FOCUS_LOST:   return WindowEvent::KEYBOARD_FOCUS_LOST;
    case SDL_WINDOWEVENT_CLOSE:     return WindowEvent::CLOSE;
  }
}

} // namespace

bool EventHandler::EventConverter::convert_window(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_WINDOWEVENT ) return false;

  auto wtype = window_event_type(ev->window.event);
  if( wtype == WindowEvent::NONE ) return false;

  out.type = Event::WINDOW_EVENT;
  out.timestamp = ev->window.timestamp;
  out.window.event = wtype;
  out.window.data1 = ev->window.data1;
  out.window.data2 = ev->window.data2;
  out.window.window_id = ev->window.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_key(const SDL_Event* ev, Event& out)
{
  if( ev->type == SDL_KEYDOWN ) {
    out.type = Event::KEY_PRESS;
  } else if( ev->type == SDL_KEYUP ) {
    out.type = Event::KEY_RELEASE;
  } else {
    return false;
  }
  out.timestamp = ev->key.timestamp;
  out.key.scan_code = ev->key.keysym.scancode;
  out.key.sym = ev->key.keysym.sym;
  out.key.mod = ev->key.keysym.mod;
  out.key.state = ev->key.state;
  out.key.repeat = ev->key.repeat;
  out.key.window_id = ev->key.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_mouse_motion(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_MOUSEMOTION ) return false;
  out.type = Event::MOUSE_MOTION;
  out.timestamp = ev->motion.timestamp;
  out.motion.id = ev->motion.which;
  out.motion.x = ev->motion.x;
  out.motion.y = ev->motion.y;
  out.motion.x_rel = ev->motion.xrel;
  out.motion.y_rel = ev->motion.yrel;
  out.motion.state = ev->motion.state;
  out.motion.window_id = ev->motion.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_mouse_button(const SDL_Event* ev, Event& out)
{
  Event::EventType etype;
  if( ev->type == SDL_MOUSEBUTTONDOWN ) {
    etype = Event::MOUSE_BUTTON_CLICK;
  } else if( ev->type == SDL_MOUSEBUTTONUP ) {
    etype = Event::MOUSE_BUTTON_RELEASE;
  } else {
    return false;
  }

  uint8 button = ev->button.button;
  if( button != SDL_BUTTON_LEFT && button != SDL_BUTTON_MIDDLE &&
      button != SDL_BUTTON_RIGHT && button != SDL_BUTTON_X1 &&
      button != SDL_BUTTON_X2 ) {
    return false;
  }

  out.type = etype;
  out.timestamp = ev->button.timestamp;
  out.mouse.id = ev->button.which;
  out.mouse.x = ev->button.x;
  out.mouse.y = ev->button.y;
  out.mouse.button = button;
  out.mouse.state = ev->button.state;
  out.mouse.clicks = ev->button.clicks;
  out.mouse.window_id = ev->button.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_mouse_wheel(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_MOUSEWHEEL ) return false;
  out.type = Event::MOUSE_WHEEL;
  out.timestamp = ev->wheel.timestamp;
  out.wheel.id = ev->wheel.which;
  out.wheel.x = ev->wheel.x;
  out.wheel.y = ev->wheel.y;
  out.wheel.window_id = ev->wheel.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_finger(const SDL_Event* ev, Event& out)
{
  if( ev->type == SDL_FINGERMOTION ) {
    out.type = Event::FINGER_MOTION;
  } else if( ev->type == SDL_FINGERDOWN ) {
    out.type = Event::FINGER_PRESS;
  } else if( ev->type == SDL_FINGERUP ) {
    out.type = Event::FINGER_RELEASE;
  } else {
    return false;
  }
  out.timestamp = ev->tfinger.timestamp;
  out.touch.id = ev->tfinger.touchId;
  out.touch.finger.id = ev->tfinger.fingerId;
  out.touch.x = ev->tfinger.x;
  out.touch.y = ev->tfinger.y;
  out.touch.dx = ev->tfinger.dx;
  out.touch.dy = ev->tfinger.dy;
  out.touch.pressure = ev->tfinger.pressure;
  return true;
}

bool EventHandler::EventConverter::convert_gesture(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_MULTIGESTURE ) return false;
  out.type = Event::MULTI_FINGER_GESTURE;
  out.timestamp = ev->mgesture.timestamp;
  out.gesture.id = ev->mgesture.touchId;
  out.gesture.dTheta = ev->mgesture.dTheta;
  out.gesture.dDist = ev->mgesture.dDist;
  out.gesture.x = ev->mgesture.x;
  out.gesture.y = ev->mgesture.y;
  out.gesture.num_fingers = ev->mgesture.numFingers;
  return true;
}

bool EventHandler::EventConverter::convert_drop(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_DROPFILE ) return false;
  out.type = Event::DROP_FILE;
  out.timestamp = nom::ticks();
  out.drop.file_path = ev->drop.file;
  return true;
}

bool EventHandler::EventConverter::convert_text_input(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_TEXTINPUT ) return false;
  out.type = Event::TEXT_INPUT;
  out.timestamp = ev->text.timestamp;
  nom::copy_string(ev->text.text, out.text.text);
  out.text.window_id = ev->text.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_text_editing(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_TEXTEDITING ) return false;
  out.type = Event::TEXT_EDITING;
  out.timestamp = ev->edit.timestamp;
  out.edit.start = ev->edit.start;
  out.edit.length = ev->edit.length;
  nom::copy_string(ev->edit.text, out.edit.text);
  out.edit.window_id = ev->edit.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_render_targets_reset(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_RENDER_TARGETS_RESET ) return false;
  out.type = Event::RENDER_TARGETS_RESET;
  out.timestamp = nom::ticks();
  return true;
}

bool EventHandler::EventConverter::convert_user(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_USEREVENT ) return false;
  out.type = Event::USER_EVENT;
  out.timestamp = ev->user.timestamp;
  out.user.code = ev->user.code;
  out.user.data1 = ev->user.data1;
  out.user.data2 = ev->user.data2;
  out.user.window_id = ev->user.windowID;
  return true;
}

bool EventHandler::EventConverter::convert_joystick_device(const SDL_Event* ev, Event& out)
{
  if( ev->type == SDL_JOYDEVICEADDED ) {
    out.type = Event::JOYSTICK_ADDED;
    out.timestamp = ev->jdevice.timestamp;
    out.jdevice.id = ev->jdevice.which;
    return true;
  } else if( ev->type == SDL_JOYDEVICEREMOVED ) {
    out.type = Event::JOYSTICK_REMOVED;
    out.timestamp = ev->jdevice.timestamp;
    out.jdevice.id = ev->jdevice.which;
    return true;
  }
  return false;
}

bool EventHandler::EventConverter::convert_joystick_button(const SDL_Event* ev, Event& out)
{
  if( ev->type == SDL_JOYBUTTONDOWN ) {
    out.type = Event::JOYSTICK_BUTTON_PRESS;
  } else if( ev->type == SDL_JOYBUTTONUP ) {
    out.type = Event::JOYSTICK_BUTTON_RELEASE;
  } else {
    return false;
  }
  out.timestamp = ev->jbutton.timestamp;
  out.jbutton.id = ev->jbutton.which;
  out.jbutton.button = ev->jbutton.button;
  out.jbutton.state = ev->jbutton.state;
  return true;
}

bool EventHandler::EventConverter::convert_joystick_axis(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_JOYAXISMOTION ) return false;
  out.type = Event::JOYSTICK_AXIS_MOTION;
  out.timestamp = ev->jaxis.timestamp;
  out.jaxis.id = ev->jaxis.which;
  out.jaxis.axis = ev->jaxis.axis;
  out.jaxis.value = ev->jaxis.value;
  return true;
}

bool EventHandler::EventConverter::convert_joystick_hat(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_JOYHATMOTION ) return false;
  out.type = Event::JOYSTICK_HAT_MOTION;
  out.timestamp = ev->jhat.timestamp;
  out.jhat.id = ev->jhat.which;
  out.jhat.hat = ev->jhat.hat;
  out.jhat.value = ev->jhat.value;
  return true;
}

bool EventHandler::EventConverter::convert_controller_device(const SDL_Event* ev, Event& out)
{
  if( ev->type == SDL_CONTROLLERDEVICEADDED ) {
    out.type = Event::GAME_CONTROLLER_ADDED;
    out.timestamp = ev->cdevice.timestamp;
    out.cdevice.id = ev->cdevice.which;
    return true;
  } else if( ev->type == SDL_CONTROLLERDEVICEREMOVED ) {
    out.type = Event::GAME_CONTROLLER_REMOVED;
    out.timestamp = ev->cdevice.timestamp;
    out.cdevice.id = ev->cdevice.which;
    return true;
  } else if( ev->type == SDL_CONTROLLERDEVICEREMAPPED ) {
    out.type = Event::GAME_CONTROLLER_REMAPPED;
    out.timestamp = ev->cdevice.timestamp;
    out.cdevice.id = ev->cdevice.which;
    return true;
  }
  return false;
}

bool EventHandler::EventConverter::convert_controller_button(const SDL_Event* ev, Event& out)
{
  if( ev->type == SDL_CONTROLLERBUTTONDOWN ) {
    out.type = Event::GAME_CONTROLLER_BUTTON_PRESS;
  } else if( ev->type == SDL_CONTROLLERBUTTONUP ) {
    out.type = Event::GAME_CONTROLLER_BUTTON_RELEASE;
  } else {
    return false;
  }
  out.timestamp = ev->cbutton.timestamp;
  out.cbutton.id = ev->cbutton.which;
  out.cbutton.button = ev->cbutton.button;
  out.cbutton.state = ev->cbutton.state;
  return true;
}

bool EventHandler::EventConverter::convert_controller_axis(const SDL_Event* ev, Event& out)
{
  if( ev->type != SDL_CONTROLLERAXISMOTION ) return false;
  out.type = Event::GAME_CONTROLLER_AXIS_MOTION;
  out.timestamp = ev->caxis.timestamp;
  out.caxis.id = ev->caxis.which;
  out.caxis.axis = ev->caxis.axis;
  out.caxis.value = ev->caxis.value;
  return true;
}

// ---------------------------------------------------------------------------
// DeviceLifecycleManager — joystick / game controller lifecycle
// ---------------------------------------------------------------------------

EventHandler::DeviceLifecycleManager::DeviceLifecycleManager() = default;
EventHandler::DeviceLifecycleManager::~DeviceLifecycleManager() = default;

bool EventHandler::DeviceLifecycleManager::enable_joystick(EventHandler& owner)
{
  (void)owner;
  if( nom::init_joystick_subsystem() == false ) {
    return false;
  }
  this->handler = new JoystickEventHandler();
  if( this->handler == nullptr ) {
    nom::set_error(nom::OUT_OF_MEMORY_ERR);
    return false;
  }
  this->type = SDL_JOYSTICK_EVENT_HANDLER;
  return true;
}

bool EventHandler::DeviceLifecycleManager::enable_game_controller(EventHandler& owner)
{
  (void)owner;
  if( nom::init_game_controller_subsystem() == false ) {
    return false;
  }
  this->handler = new GameControllerEventHandler();
  if( this->handler == nullptr ) {
    nom::set_error(nom::OUT_OF_MEMORY_ERR);
    return false;
  }
  this->type = GAME_CONTROLLER_EVENT_HANDLER;
  return true;
}

void EventHandler::DeviceLifecycleManager::disable_joystick(EventHandler& owner)
{
  (void)owner;
  if( this->type == SDL_JOYSTICK_EVENT_HANDLER ) {
    auto evt_handler = static_cast<JoystickEventHandler*>(this->handler);
    NOM_DELETE_PTR(evt_handler);
    this->type = NO_EVENT_HANDLER;
    nom::shutdown_joystick_subsystem();
  } else if( this->type == NO_EVENT_HANDLER ) {
    // Nothing to do
  } else {
    NOM_ASSERT_INVALID_PATH();
  }
  this->handler = nullptr;
}

void EventHandler::DeviceLifecycleManager::disable_game_controller(EventHandler& owner)
{
  (void)owner;
  if( this->type == GAME_CONTROLLER_EVENT_HANDLER ) {
    auto evt_handler = static_cast<GameControllerEventHandler*>(this->handler);
    NOM_DELETE_PTR(evt_handler);
    this->type = NO_EVENT_HANDLER;
    nom::shutdown_game_controller_subsystem();
  } else if( this->type == NO_EVENT_HANDLER ) {
    // Nothing to do
  } else {
    NOM_ASSERT_INVALID_PATH();
  }
  this->handler = nullptr;
}

void EventHandler::DeviceLifecycleManager::shutdown(EventHandler& owner)
{
  if( this->type == SDL_JOYSTICK_EVENT_HANDLER ) {
    this->disable_joystick(owner);
  } else if( this->type == GAME_CONTROLLER_EVENT_HANDLER ) {
    this->disable_game_controller(owner);
  }
}

bool EventHandler::DeviceLifecycleManager::handle_device_event(
    const SDL_Event* ev, EventHandler& owner)
{
  if( this->handler == nullptr ) return false;

  if( this->type == SDL_JOYSTICK_EVENT_HANDLER ) {

    auto evt_handler = static_cast<JoystickEventHandler*>(this->handler);
    NOM_ASSERT(evt_handler != nullptr);

    if( ev->type == SDL_JOYDEVICEADDED ) {
      Event dev_event;
      if( EventConverter::convert_joystick_device(ev, dev_event) ) {
        auto dev_index = dev_event.jdevice.id;
        auto joy_dev = evt_handler->add_joystick(dev_index);
        if( joy_dev != nullptr ) {
          auto dev_id = joy_dev->device_id();
          NOM_LOG_INFO( NOM_LOG_CATEGORY_EVENT,
                        "Registered joystick instance ID",
                        dev_id, "for", joy_dev->name() );
        } else {
          NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                        "Failed to register joystick:", nom::error() );
        }
        return true;
      }
    } else if( ev->type == SDL_JOYDEVICEREMOVED ) {
      Event dev_event;
      if( EventConverter::convert_joystick_device(ev, dev_event) ) {
        auto dev_id = dev_event.jdevice.id;
        if( evt_handler->remove_joystick(dev_id) == true ) {
          NOM_LOG_INFO( NOM_LOG_CATEGORY_EVENT,
                        "Removing registered instance ID", dev_id );
        } else {
          NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                        "Failed to remove registered instance ID:",
                        nom::error() );
        }
        return true;
      }
    }

  } else if( this->type == GAME_CONTROLLER_EVENT_HANDLER ) {

    auto evt_handler = static_cast<GameControllerEventHandler*>(this->handler);
    NOM_ASSERT(evt_handler != nullptr);

    if( ev->type == SDL_CONTROLLERDEVICEADDED ) {
      Event dev_event;
      if( EventConverter::convert_controller_device(ev, dev_event) ) {
        auto dev_index = dev_event.cdevice.id;
        auto joy_dev = evt_handler->add_joystick(dev_index);
        if( joy_dev != nullptr ) {
          auto dev_id = joy_dev->device_id();
          NOM_LOG_INFO( NOM_LOG_CATEGORY_EVENT,
                        "Registered game controller instance ID",
                        dev_id, "for", joy_dev->name() );
        } else {
          NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                        "Failed to register game controller:", nom::error() );
        }
        return true;
      }
    } else if( ev->type == SDL_CONTROLLERDEVICEREMOVED ) {
      Event dev_event;
      if( EventConverter::convert_controller_device(ev, dev_event) ) {
        auto dev_id = dev_event.cdevice.id;
        if( evt_handler->remove_joystick(dev_id) == true ) {
          NOM_LOG_INFO( NOM_LOG_CATEGORY_EVENT,
                        "Removing registered instance ID", dev_id );
        } else {
          NOM_LOG_ERR(  NOM_LOG_CATEGORY_APPLICATION,
                        "Failed to remove registered instance ID:",
                        nom::error() );
        }
        return true;
      }
    }
  }

  (void)owner;
  return false;
}

// ---------------------------------------------------------------------------
// EventDispatcher — unified queue + watcher dispatch
// ---------------------------------------------------------------------------

EventHandler::EventDispatcher::EventDispatcher() = default;
EventHandler::EventDispatcher::~EventDispatcher() = default;

void EventHandler::EventDispatcher::dispatch(const Event& ev, EventHandler& owner)
{
  owner.events_.emplace_back(ev);

  nom::size_type num_events = owner.events_.size();
  if( num_events > this->max_events_count ) {
    this->max_events_count = num_events;
  }

  auto& evt_watch = owner.event_watchers_;
  for( auto itr = evt_watch.begin(); itr != evt_watch.end(); ++itr ) {
    if( (*itr)->callback != nullptr ) {
      (*itr)->callback.operator()(ev, (*itr)->data1);
    }
  }
}

// ---------------------------------------------------------------------------
// EventHandler — public API
// ---------------------------------------------------------------------------

EventHandler::EventHandler()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_EVENT,
                      NOM_LOG_PRIORITY_VERBOSE );
}

EventHandler::~EventHandler()
{
  NOM_LOG_TRACE_PRIO( NOM_LOG_CATEGORY_TRACE_EVENT,
                      NOM_LOG_PRIORITY_VERBOSE );

  auto enable_report = nom::hint("NOM_EVENT_QUEUE_STATISTICS");
  if( nom::string_to_int(enable_report.c_str()) != 0 ) {
    NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_APPLICATION,
                    "num_events:", this->num_events() );
    NOM_LOG_DEBUG(  NOM_LOG_CATEGORY_APPLICATION,
                    "max_events_count:", this->dispatcher_.max_events_count );
  }

  this->device_mgr_.shutdown(*this);
}

nom::size_type EventHandler::num_events() const
{
  return this->events_.size();
}

nom::size_type EventHandler::num_event_watchers() const
{
  return this->event_watchers_.size();
}

JoystickEventHandler* EventHandler::joystick_event_handler() const
{
  return static_cast<JoystickEventHandler*>(this->device_mgr_.handler);
}

GameControllerEventHandler* EventHandler::game_controller_event_handler() const
{
  return static_cast<GameControllerEventHandler*>(this->device_mgr_.handler);
}

EventHandler::JoystickHandlerType
EventHandler::joystick_event_type() const
{
  return this->device_mgr_.type;
}

bool EventHandler::enable_joystick_polling()
{
  return this->device_mgr_.enable_joystick(*this);
}

bool EventHandler::enable_game_controller_polling()
{
  return this->device_mgr_.enable_game_controller(*this);
}

void EventHandler::disable_joystick_polling()
{
  this->device_mgr_.disable_joystick(*this);
}

void EventHandler::disable_game_controller_polling()
{
  this->device_mgr_.disable_game_controller(*this);
}

bool EventHandler::poll_event(Event& ev)
{
  return this->pop_event(ev) ? true : false;
}

void EventHandler::append_event_watch(const event_filter& filter, void* data)
{
  if( filter == nullptr ) {
    return;
  }

  auto event_watch = nom::make_unique<event_watcher>();
  if( event_watch == nullptr ) {
    NOM_ASSERT_INVALID_PATH();
    return;
  }

  event_watch->callback = filter;
  event_watch->data1 = data;
  this->event_watchers_.push_back( std::move(event_watch) );
}

void EventHandler::remove_event_watch(const event_filter& filter)
{
  if( filter == nullptr ) {
    return;
  }

  auto& evt_watch = this->event_watchers_;
  for( auto itr = evt_watch.begin(); itr != evt_watch.end(); ++itr ) {

    event_filter* callback = (*itr)->callback.target<event_filter>();
    const event_filter* arg = filter.target<event_filter>();

    if( callback == arg ) {
      evt_watch.erase(itr);
      return;
    }
  }
}

void EventHandler::remove_event_watchers()
{
  this->event_watchers_.clear();
}

void EventHandler::push_event(const Event& ev)
{
  this->dispatcher_.dispatch(ev, *this);
}

bool EventHandler::pop_event(Event& ev)
{
  bool result = false;

  if( this->events_.empty() == true ) {

    this->process_events();

    result = false;
  }

  if( this->events_.empty() == false ) {

    ev = this->events_.front();

    this->events_.pop_front();
    result = true;
  }

  return result;
}

// ---------------------------------------------------------------------------
// EventHandler — event processing pipeline
// ---------------------------------------------------------------------------

void EventHandler::process_events()
{
  int result = 1;
  SDL_Event ev;

  SDL_PumpEvents();

  while( result > 0 ) {
    result =
      SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);

    if( result < 0 ) {
      NOM_ASSERT_INVALID_PATH();
    } else if( result > 0 ) {

      if( this->device_mgr_.handler != nullptr ) {
        auto type = this->joystick_event_type();
        if( type == SDL_JOYSTICK_EVENT_HANDLER ) {
          this->process_joystick_event(&ev);
        } else if( type == GAME_CONTROLLER_EVENT_HANDLER ) {
          this->process_game_controller_event(&ev);
        }
      }

      this->process_event(&ev);
    }
  }
}

void EventHandler::process_event(const SDL_Event* ev)
{
  Event event;
  bool converted = false;

  // Keyboard
  if( !converted ) converted = EventConverter::convert_key(ev, event);

  // Mouse
  if( !converted ) converted = EventConverter::convert_mouse_motion(ev, event);
  if( !converted ) converted = EventConverter::convert_mouse_button(ev, event);
  if( !converted ) converted = EventConverter::convert_mouse_wheel(ev, event);

  // Window / Quit
  if( !converted ) converted = EventConverter::convert_quit(ev, event);
  if( !converted ) converted = EventConverter::convert_window(ev, event);

  // Touch / Gesture
  if( !converted ) converted = EventConverter::convert_finger(ev, event);
  if( !converted ) converted = EventConverter::convert_gesture(ev, event);

  // Text
  if( !converted ) converted = EventConverter::convert_text_input(ev, event);
  if( !converted ) converted = EventConverter::convert_text_editing(ev, event);

  // Drag & drop
  if( !converted ) converted = EventConverter::convert_drop(ev, event);

  // Render
  if( !converted ) converted = EventConverter::convert_render_targets_reset(ev, event);

  // User
  if( !converted ) converted = EventConverter::convert_user(ev, event);

  // Joystick (non-lifecycle; lifecycle handled separately)
  if( !converted ) converted = EventConverter::convert_joystick_button(ev, event);
  if( !converted ) converted = EventConverter::convert_joystick_axis(ev, event);
  if( !converted ) converted = EventConverter::convert_joystick_hat(ev, event);

  // Game controller (non-lifecycle; lifecycle handled separately)
  if( !converted ) converted = EventConverter::convert_controller_button(ev, event);
  if( !converted ) converted = EventConverter::convert_controller_axis(ev, event);

  // Joystick / controller device events (still need to be dispatched)
  if( !converted ) converted = EventConverter::convert_joystick_device(ev, event);
  if( !converted ) converted = EventConverter::convert_controller_device(ev, event);

  if( converted ) {
    this->dispatcher_.dispatch(event, *this);
  }
}

void EventHandler::process_joystick_event(const SDL_Event* ev)
{
  NOM_ASSERT(this->joystick_event_type() == SDL_JOYSTICK_EVENT_HANDLER);
  this->device_mgr_.handle_device_event(ev, *this);
}

void EventHandler::process_game_controller_event(const SDL_Event* ev)
{
  NOM_ASSERT(this->joystick_event_type() == GAME_CONTROLLER_EVENT_HANDLER);
  this->device_mgr_.handle_device_event(ev, *this);
}

// ---------------------------------------------------------------------------
// Event queue flushing
// ---------------------------------------------------------------------------

void EventHandler::flush_event(Event::EventType type)
{
  for( auto itr = this->events_.begin(); itr != this->events_.end(); ++itr ) {
    if( (*itr).type == type ) {
      this->events_.erase(itr);
      return;
    }
  }
}

void EventHandler::flush_events(Event::EventType type)
{
  for( auto itr = this->events_.begin(); itr != this->events_.end(); ++itr ) {
    if( (*itr).type == type ) {
      this->events_.erase(itr);
    }
  }
}

void EventHandler::flush_events()
{
  this->events_.clear();
}

// ---------------------------------------------------------------------------
// Convenience event factory functions
// ---------------------------------------------------------------------------

Event create_key_press(int32 sym, uint16 mod, uint8 repeat)
{
  nom::Event result;
  result.type = Event::KEY_PRESS;
  result.timestamp = nom::ticks();
  result.key.scan_code = SDL_GetScancodeFromKey(sym);
  result.key.sym = sym;
  result.key.mod = mod;
  result.key.state = InputState::PRESSED;
  result.key.repeat = repeat;

  return result;
}

Event create_key_release(int32 sym, uint16 mod, uint8 repeat)
{
  nom::Event result;
  result.type = Event::KEY_RELEASE;
  result.timestamp = nom::ticks();
  result.key.scan_code = SDL_GetScancodeFromKey(sym);
  result.key.sym = sym;
  result.key.mod = mod;
  result.key.state = InputState::RELEASED;
  result.key.repeat = repeat;

  return result;
}

Event create_mouse_button_click(uint8 button, uint8 clicks, uint32 window_id)
{
  nom::Event result;
  result.type = Event::MOUSE_BUTTON_CLICK;
  result.timestamp = nom::ticks();
  result.mouse.button = button;
  result.mouse.state = InputState::PRESSED;
  result.mouse.clicks = clicks;
  result.mouse.window_id = window_id;

  return result;
}

Event create_mouse_button_release(uint8 button, uint8 clicks, uint32 window_id)
{
  nom::Event result;
  result.type = Event::MOUSE_BUTTON_RELEASE;
  result.timestamp = nom::ticks();
  result.mouse.button = button;
  result.mouse.state = InputState::RELEASED;
  result.mouse.clicks = clicks;
  result.mouse.window_id = window_id;

  return result;
}

Event create_joystick_button_press(JoystickID id, uint8 button)
{
  nom::Event result;
  result.type = Event::JOYSTICK_BUTTON_PRESS;
  result.timestamp = nom::ticks();
  result.jbutton.id = id;
  result.jbutton.button = button;
  result.jbutton.state = InputState::PRESSED;

  return result;
}

Event create_joystick_button_release(JoystickID id, uint8 button)
{
  nom::Event result;
  result.type = Event::JOYSTICK_BUTTON_RELEASE;
  result.timestamp = nom::ticks();
  result.jbutton.id = id;
  result.jbutton.button = button;
  result.jbutton.state = InputState::RELEASED;

  return result;
}

Event create_joystick_hat_motion(JoystickID id, uint8 hat, uint8 value)
{
  nom::Event result;
  result.type = Event::JOYSTICK_HAT_MOTION;
  result.timestamp = nom::ticks();
  result.jhat.id = id;
  result.jhat.hat = hat;
  result.jhat.value = value;

  return result;
}

Event create_game_controller_button_press(JoystickID id, uint8 button)
{
  nom::Event result;
  result.type = Event::GAME_CONTROLLER_BUTTON_PRESS;
  result.timestamp = nom::ticks();
  result.cbutton.id = id;
  result.cbutton.button = button;
  result.cbutton.state = InputState::PRESSED;

  return result;
}

Event create_game_controller_button_release(JoystickID id, uint8 button)
{
  nom::Event result;
  result.type = Event::GAME_CONTROLLER_BUTTON_RELEASE;
  result.timestamp = nom::ticks();
  result.cbutton.id = id;
  result.cbutton.button = button;
  result.cbutton.state = InputState::RELEASED;

  return result;
}

Event
create_user_event(int32 code, void* data1, void* data2, uint32 window_id)
{
  nom::Event result;
  result.type = Event::USER_EVENT;
  result.timestamp = nom::ticks();
  result.user.code = code;
  result.user.data1 = data1;
  result.user.data2 = data2;
  result.user.window_id = window_id;

  return result;
}

Event create_quit_event(void* data1, void* data2)
{
  nom::Event result;
  result.type = Event::QUIT_EVENT;
  result.timestamp = nom::ticks();
  result.quit.data1 = data1;
  result.quit.data2 = data2;

  return result;
}

} // namespace nom
