#include "VslinkParser.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
namespace vslink {
namespace {
using SV = std::string_view;
bool space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; }
SV trim(SV s) { while (!s.empty() && space(s.front())) s.remove_prefix(1); while (!s.empty() && space(s.back())) s.remove_suffix(1); return s; }
bool number(SV s, double &v) {
  s = trim(s);
  if (s.empty()) return false;
  if (s.front() == '+') { s.remove_prefix(1); if (s.empty() || s.front() == '-') return false; }
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, std::chars_format::general);
  return r.ec == std::errc{} && r.ptr == s.data() + s.size() && std::isfinite(v);
}
struct Mapping { SV name; uint16_t bit; };
constexpr Mapping xbox[] = {{"A",4096},{"B",8192},{"X",16384},{"Y",32768},
 {"LEFT_BUMPER",256},{"LB",256},{"RIGHT_BUMPER",512},{"RB",512},
 {"BACK",32},{"START",16},{"LEFT_THUMB",64},{"LS",64},{"RIGHT_THUMB",128},{"RS",128},
 {"HOME",1024},{"GUIDE",1024},{"XBOX_BUTTON",1024},
 // Explicit compatibility extensions requested by the receiver specification.
 {"LEFT_SHOULDER",256},{"RIGHT_SHOULDER",512}};
constexpr Mapping ps[] = {{"CROSS",4096},{"CIRCLE",8192},{"SQUARE",16384},{"TRIANGLE",32768},
 {"L1",256},{"R1",512},{"SHARE",32},{"OPTIONS",16},{"L3",64},{"R3",128},{"PS_BUTTON",1024},{"TOUCHPAD",32}};
constexpr Mapping dpad[] = {{"DPAD_UP",1},{"DPAD_DOWN",2},{"DPAD_LEFT",4},{"DPAD_RIGHT",8}};
template<size_t N> bool button(SV name, const Mapping (&map)[N], Command &c) {
  for (auto &m : map) if (name == m.name) { c.kind = CommandKind::Button; c.button = m.bit; return true; }
  return false;
}
}
bool VslinkParser::parse(SV raw, ControllerType type, Command &out) {
  if (raw.empty() || raw.size() > MaxCommand) return false;
  // Protocol commands are ASCII, a valid UTF-8 subset. Reject embedded NUL/non-ASCII.
  for (unsigned char c : raw) if (c == 0 || c >= 128) return false;
  SV s = trim(raw);
  Command c;
  if (s == "RESET") { out = c; return true; }
  constexpr SV prefix = "Change gamepad type to:";
  if (s.substr(0, prefix.size()) == prefix) {
    SV t = trim(s.substr(prefix.size()));
    char upper[5]{};
    if (t.size() > 4) return false;
    for (size_t i=0;i<t.size();++i) upper[i] = t[i] >= 'a' && t[i] <= 'z' ? t[i]-32 : t[i];
    if (SV(upper,t.size()) == "PS") c.type = ControllerType::PlayStation;
    else if (SV(upper,t.size()) != "XBOX") return false;
    c.kind = CommandKind::Type; out = c; return true;
  }
  auto last = s.rfind('_');
  if (last == SV::npos) return false;
  SV name = s.substr(0,last), value = s.substr(last+1);
  bool digital = value == "PRESS" || value == "RELEASE";
  if (digital) {
    c.pressed = value == "PRESS";
    if (button(name,dpad,c) || (type == ControllerType::Xbox ? button(name,xbox,c) : button(name,ps,c))) { out=c; return true; }
  }
  bool left = name == "LEFT_TRIGGER" || name == "LT" || name == "L2";
  bool right = name == "RIGHT_TRIGGER" || name == "RT" || name == "R2";
  if (left || right) {
    if (digital) {
      bool valid = type == ControllerType::Xbox ? (name != "L2" && name != "R2") : (name == "L2" || name == "R2");
      if (!valid) return false;
      c.x = c.pressed ? 1 : 0;
    } else if (!number(value,c.x)) return false;
    c.kind=CommandKind::Trigger; c.right=right; out=c; return true;
  }
  for (SV side : {SV("LEFT_JOYSTICK_"), SV("RIGHT_JOYSTICK_")}) {
    if (s.substr(0,side.size()) != side) continue;
    SV tail = s.substr(side.size());
    c.kind=CommandKind::Stick; c.right=side.front()=='R';
    if (tail == "RESET") {}
    else if (tail == "LEFT") c.x=-1;
    else if (tail == "RIGHT") c.x=1;
    // DS4 discrete source directions are opposite numerically. Translate to Xbox up+.
    else if (tail == "UP") c.y=1;
    else if (tail == "DOWN") c.y=-1;
    else {
      auto split = tail.find('_');
      if (split == SV::npos || !number(tail.substr(0,split),c.x) || !number(tail.substr(split+1),c.y)) return false;
      // Continuous PS coordinates are passed unchanged by the reference; DS4 Y is down+.
      if (type == ControllerType::PlayStation) c.y=-c.y;
    }
    out=c; return true;
  }
  return false;
}
}
