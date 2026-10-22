#include "UsbGamepad.hpp"
#include "usb_device.hpp"
#include "sdkconfig.h"
#include "esp_log.h"
#include <array>
namespace vslink {
UsbGamepad::UsbGamepad() = default;
UsbGamepad::~UsbGamepad() = default;
bool UsbGamepad::initialize() {
  espp::UsbDevice::Config config;
  config.manufacturer="VSLink";
  config.product="VSLink BLE Gamepad";
#if CONFIG_VSLINK_USB_XINPUT
  config.xinput=espp::UsbDevice::XInputFunction{};
#else
  config.vid=CONFIG_VSLINK_HID_VID; config.pid=CONFIG_VSLINK_HID_PID;
  espp::UsbDevice::HidFunction hid;
  hid.poll_interval_ms=4;
  // 16 button bits (Xbox positions, including four independent D-pad bits),
  // four signed 16-bit axes, two unsigned 8-bit triggers. No report ID.
  hid.report_descriptor={
    0x05,0x01,0x09,0x05,0xA1,0x01,
    0x05,0x09,0x19,0x01,0x29,0x10,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x10,0x81,0x02,
    0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x33,0x09,0x34,
    0x16,0x00,0x80,0x26,0xFF,0x7F,0x75,0x10,0x95,0x04,0x81,0x02,
    0x09,0x32,0x09,0x35,0x15,0x00,0x26,0xFF,0x00,0x75,0x08,0x95,0x02,0x81,0x02,0xC0};
  config.hid=hid;
#endif
  device_=std::make_unique<espp::UsbDevice>(config);
  std::error_code ec;
  bool ok=device_->initialize(ec);
  if (!ok) ESP_LOGE("USB","Initialization failed: %s",ec.message().c_str());
  return ok;
}
bool UsbGamepad::send(const GamepadState &s) {
#if CONFIG_VSLINK_USB_XINPUT
  espp::xinput::GamepadState out;
  out.buttons=s.buttons; out.left_trigger=s.left_trigger; out.right_trigger=s.right_trigger;
  out.lx=s.lx; out.ly=s.ly; out.rx=s.rx; out.ry=s.ry;
  return device_->update_xinput_state(out);
#else
  std::array<uint8_t,12> out{};
  auto put=[&](unsigned i,int v) { out[i]=v & 255; out[i+1]=(static_cast<unsigned>(v)>>8)&255; };
  put(0,s.buttons); put(2,s.lx);
  // HID Y grows downwards; saturate rather than overflow when negating -32768.
  auto y=[](int v) { return v == -32768 ? 32767 : -v; };
  put(4,y(s.ly)); put(6,s.rx); put(8,y(s.ry));
  out[10]=s.left_trigger; out[11]=s.right_trigger;
  return device_->write_hid_report(0,out);
#endif
}
}
