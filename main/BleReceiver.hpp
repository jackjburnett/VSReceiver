#pragma once
#include <cstddef>
namespace vslink {
class BleReceiver {
 public:
  using Write = bool (*)(const char *,size_t);
  using Connection = void (*)(bool);
  static void start(Write,Connection);
};
}
