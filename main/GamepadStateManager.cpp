#include "GamepadStateManager.hpp"
#include "VslinkParser.hpp"
#include <algorithm>
#include <cmath>
namespace vslink {
// Python round(): ties to even, independent of the platform rounding mode.
static int rounded(double v) {
  double lo = std::floor(v), f = v - lo;
  return static_cast<int>(lo + (f > .5 || (f == .5 && std::fmod(lo, 2) != 0)));
}
static int16_t axis(double v) {
  v = std::clamp(v, -1.0, 1.0);
  return static_cast<int16_t>(rounded(v * 32767));
}
bool GamepadStateManager::receive(std::string_view text) {
  Command c;
  if (!VslinkParser::parse(text, type_, c)) return false;
  apply(c);
  return true;
}
void GamepadStateManager::reset(bool new_connection) {
  state_ = {};
  if (new_connection) type_ = ControllerType::Xbox;
}
void GamepadStateManager::apply(const Command &c) {
  switch (c.kind) {
    case CommandKind::Reset: reset(); break;
    case CommandKind::Type: reset(); type_ = c.type; break;
    case CommandKind::Button:
      // DS4 reference uses a single cardinal hat: any release centres it.
      if (type_ == ControllerType::PlayStation && (c.button & 15)) state_.buttons &= ~15u;
      if (c.pressed) state_.buttons |= c.button;
      else state_.buttons &= static_cast<uint16_t>(~c.button);
      break;
    case CommandKind::Stick:
      (c.right ? state_.rx : state_.lx) = axis(c.x);
      (c.right ? state_.ry : state_.ly) = axis(c.y);
      break;
    case CommandKind::Trigger:
      (c.right ? state_.right_trigger : state_.left_trigger) =
        static_cast<uint8_t>(rounded(std::clamp(c.x, 0.0, 1.0) * 255));
      break;
  }
}
}
