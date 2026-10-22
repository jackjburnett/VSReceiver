#pragma once
#include <cstdint>
#include <string_view>
namespace vslink {
enum class ControllerType { Xbox, PlayStation };
struct GamepadState {
  uint16_t buttons = 0;
  uint8_t left_trigger = 0, right_trigger = 0;
  int16_t lx = 0, ly = 0, rx = 0, ry = 0;
};
enum class CommandKind { Button, Stick, Trigger, Reset, Type };
struct Command {
  CommandKind kind = CommandKind::Reset;
  uint16_t button = 0;
  bool pressed = false, right = false;
  double x = 0, y = 0;
  ControllerType type = ControllerType::Xbox;
};
class GamepadStateManager {
 public:
  bool receive(std::string_view text);
  void apply(const Command &command);
  void reset(bool new_connection = false);
  const GamepadState &state() const { return state_; }
  ControllerType type() const { return type_; }
 private:
  GamepadState state_{};
  ControllerType type_ = ControllerType::Xbox;
};
// All state operations are confined to the controller task (single writer).
class GamepadOutput {
 public:
  virtual ~GamepadOutput() = default;
  virtual bool send(const GamepadState &) = 0;
};
}
