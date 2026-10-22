# Troubleshooting

| Symptom | Remedy |
|---|---|
| Native COM port disappears | Expected after gamepad starts. Use BOOT+RESET for ROM download, independent UART0 for logs. |
| Cannot flash | Hold BOOT, tap RESET, release BOOT. Choose ROM COM port and a USB data cable. Reset after flashing. |
| USB boot loop / missing gamepad | Primary console must be UART0, secondary NONE. USB-Serial-JTAG cannot share the gamepad PHY. Check ESP32-S3/native OTG connection. |
| Unknown USB device in XInput mode | Confirm 045E:028E, pinned ESPP, TinyUSB 0.21, esp_tinyusb 2.x, no CDC/vendor/HID. Retain ESPP's `-u usbd_app_driver_get_cb` linker option and separate IN/OUT endpoint numbers. Inspect UART errors and use a direct USB port. |
| No gamepad input | Test A_PRESS/A_RELEASE with BLE central; check UUID/properties and selected controller type. Verify XUSB driver binding. |
| App cannot find receiver | Discover by service UUID, only one central supported. Close BLE scanners, refresh phone service cache/permissions. Select BLE transport, not WebSocket/old UDP. |
| Shoulder ignored | Actual source uses LEFT_BUMPER/RIGHT_BUMPER, LB/RB. Firmware also accepts LEFT_SHOULDER/RIGHT_SHOULDER explicitly. Match case. |
| PS touch features absent | Click maps to Back; no coordinates, motion, genuine DS4 identity or authentication. |
| HID function_not_supported | TinyUSB HID count must be 1; create a fresh HID sdkconfig with both defaults files. |
| HID mapping differs in game | Map manually: D-pad is buttons 1-4; Z/Rz are triggers. |
| Inputs drop under stress | Overflow deliberately clears backlog and sends neutral. Check producer rate/USB readiness before increasing queue capacity (roughly 528 bytes per slot; more backlog can add latency). |
| Slow reset after radio loss | GAP disconnect occurs after link supervision timeout; explicit disconnect flags reset immediately. USB neutral waits for endpoint readiness. |
| Dependency resolution fails | Activate IDF v5.5.1 and permit GitHub/Espressif registry access. Retain dependencies.lock; do not invent lock hashes. esp_tinyusb 3.x conflicts with this ESPP revision. |
| CLion chooses host compiler | Use activated IDF environment and Ninja; clear stale CMake cache. Host tests need a separate desktop profile. |
| No UART logs on board TX/RX | Check configured console UART0 GPIOs against XIAO schematic; printed labels may map differently. Default 115200, 3.3 V adapter, shared ground. |

Do not add pairing to fix discovery: the authoritative receiver uses plain writes. This firmware needs neither Wi-Fi nor persistent controller settings.
