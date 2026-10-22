#pragma once
#include "GamepadStateManager.hpp"
namespace vslink {
class VslinkParser {
 public:
  static constexpr unsigned MaxCommand = 512;
  static bool parse(std::string_view text, ControllerType type, Command &out);
};
}
