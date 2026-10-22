#include "BleReceiver.hpp"
#include "ConnectionManager.hpp"
#include "ControllerProcessor.hpp"
#include "UsbGamepad.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "sdkconfig.h"
namespace {
vslink::ConnectionManager<CONFIG_VSLINK_QUEUE_DEPTH> commands;
portMUX_TYPE queue_lock=portMUX_INITIALIZER_UNLOCKED;
bool write(const char *s,size_t n) {
  portENTER_CRITICAL(&queue_lock);
  bool ok=commands.enqueue(s,n);
  portEXIT_CRITICAL(&queue_lock);
  return ok;
}
void connection(bool connected) {
  portENTER_CRITICAL(&queue_lock);
  if (connected) commands.connect(); else commands.disconnect();
  portEXIT_CRITICAL(&queue_lock);
}
}
extern "C" void app_main() {
  esp_err_t rc=nvs_flash_init();
  if (rc==ESP_ERR_NVS_NO_FREE_PAGES || rc==ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase()); rc=nvs_flash_init();
  }
  ESP_ERROR_CHECK(rc);
  vslink::UsbGamepad usb;
  if (!usb.initialize()) return;
  vslink::ControllerProcessor controller;
  vslink::BleReceiver::start(write,connection);
  unsigned generation=0;
  // app_main itself is the dedicated USB/controller task. Retry each report until
  // accepted before popping another command, preserving even brief press/release pairs.
  while (true) {
    bool fresh=false;
    bool reset;
    vslink::ConnectionManager<CONFIG_VSLINK_QUEUE_DEPTH>::Event event;
    bool have=false;
    portENTER_CRITICAL(&queue_lock);
    reset=commands.take_reset(fresh);
    generation=commands.generation();
    if (!reset && controller.needs_command()) have=commands.pop(event);
    portEXIT_CRITICAL(&queue_lock);
    if (reset) controller.reset(fresh);
    else if (have) controller.receive({event.data.data(),event.length});
    // A neutral reset report must be accepted before any queued reconnect input.
    portENTER_CRITICAL(&queue_lock);
    bool current=generation==commands.generation();
    portEXIT_CRITICAL(&queue_lock);
    if (current) controller.submit(usb,static_cast<uint32_t>(xTaskGetTickCount()*portTICK_PERIOD_MS));
    // Also resend current state periodically: handles USB detach/re-enumeration.
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
