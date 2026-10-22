#pragma once
#include "GamepadStateManager.hpp"
namespace vslink {
// Single-writer report scheduling, shared by the firmware task and host tests.
class ControllerProcessor {
 public:
  bool needs_command() const { return !pending_; }
  bool receive(std::string_view command) {
    if (pending_) return false;
    if (!gamepad_.receive(command)) return false;
    pending_=true;
    return true;
  }
  void reset(bool new_connection) { gamepad_.reset(new_connection); pending_=true; }
  bool submit(GamepadOutput &usb, uint32_t now_ms) {
    if (!pending_ && now_ms-last_sent_ < 20) return true;
    pending_=true;
    if (!usb.send(gamepad_.state())) return false;
    pending_=false; last_sent_=now_ms;
    return true;
  }
  const GamepadState &state() const { return gamepad_.state(); }
 private:
  GamepadStateManager gamepad_;
  bool pending_=true;
  uint32_t last_sent_=0;
};
}
