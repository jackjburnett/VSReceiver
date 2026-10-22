#pragma once
#include "GamepadStateManager.hpp"
#include <memory>
namespace espp { class UsbDevice; }
namespace vslink {
class UsbGamepad final : public GamepadOutput {
 public:
  UsbGamepad();
  ~UsbGamepad();
  bool initialize();
  bool send(const GamepadState &) override;
 private:
  std::unique_ptr<espp::UsbDevice> device_;
};
}
