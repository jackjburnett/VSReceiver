# VSLink ESP32-S3 BLE-to-USB receiver

Firmware for Seeed Studio XIAO ESP32-S3, one BLE central controlling one USB gamepad. No physical buttons, switches, sensors or analog inputs are used. USB mode is selected at build time: Xbox 360 XInput (default) or generic HID.

Protocol audited against [VSLink](https://github.com/jackjburnett/VSLink) commit `8d22865985a017b6bf31500f7dffb83edd604c8f`. USB uses [ESPP's XInput implementation](https://github.com/esp-cpp/espp/tree/792c56e1a5cdf3938868990425d8e45cd0e6ed0c/components/usb_device/xinput_example), pinned to that commit. See [protocol](docs/PROTOCOL.md), [integration procedure](docs/INTEGRATION.md), [troubleshooting](docs/TROUBLESHOOTING.md), and [validation](docs/VALIDATION.md).

## Install and build

Use **ESP-IDF v5.5.1**, ESP32-S3 tools, CMake and Ninja. On Windows install the official ESP-IDF tools and open its ESP-IDF PowerShell terminal. With a source installation, clone v5.5.1 recursively, run `install.bat esp32s3`, then `export.ps1`. On Linux/macOS run `./install.sh esp32s3` and `. ./export.sh`. Use a short path such as `C:\GitHub\VSReceiver`.

The managed manifest fetches ESPP and dependencies automatically on first configuration. Keep the generated `dependencies.lock`. esp_tinyusb is constrained to 2.x: 3.x bundles its own TinyUSB and is incompatible with this ESPP revision's separate TinyUSB dependency. TinyUSB 0.21 is required by the five-argument endpoint API.

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COM7 flash
```

Replace COM7 with the ROM bootloader or USB-UART port. Linux commonly uses `/dev/ttyACM0` or `/dev/ttyUSB0`.

For independent configurations (defaults only apply to a fresh sdkconfig):

```powershell
idf.py -B build-xinput -D SDKCONFIG=build-xinput/sdkconfig build
idf.py -B build-hid -D SDKCONFIG=build-hid/sdkconfig -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.hid' build
idf.py -B build-hid -D SDKCONFIG=build-hid/sdkconfig -p COM7 flash
```

Alternatively use `idf.py menuconfig`: VSLink receiver selects the USB identity; TinyUSB HID count must be 1 for HID and 0 for XInput. Never enable CDC/vendor alongside XInput. HID uses development VID/PID CAFE:4010, configurable in menuconfig. Obtain assigned identifiers before distribution. Microsoft's XInput VID/PID 045E:028E are for **private emulation/testing only**, not for commercial shipping. No Xbox console authentication is implemented.

**C++ version:** parser, state, queue and host tests use C++17. ESPP's public USB API requires `std::span`, so the USB adapter compiles as C++20. A strict all-C++17 build is incompatible with this maintained ESPP implementation; the project records this necessary exception instead of replacing its USB implementation.

## Native USB, flashing and UART logs

USB-OTG and USB-Serial-JTAG share the internal PHY and GPIO19/20. The native USB-C data port becomes the gamepad and supplies **no serial console or USB JTAG** while the app runs. Primary logging uses UART0, secondary console is NONE. Normal gameplay inputs are not logged.

For native-port recovery hold **BOOT**, tap **RESET**, then release BOOT. The ROM download USB serial device appears as a COM port. Run `idf.py -p COMx flash` and reset afterwards if necessary. Holding BOOT while plugging USB also enters download mode. Application USB descriptors cannot disable this recovery path.

For serial logging use an independent **3.3 V** USB-UART adapter and common ground. Check the XIAO schematic and configured UART0 console GPIOs before wiring: board pins labelled TX/RX need not be the ESP32-S3 default UART0 pins. Select accessible console GPIOs in menuconfig if needed. Run `idf.py -p COMx monitor` (115200 default). An accessible UART0 adapter also supports flashing with manual BOOT/RESET. OTA is not implemented.

## CLion

Install ESP-IDF v5.5.1 and JetBrains ESP-IDF support. Launch CLion from an activated IDF terminal or configure its toolchain environment script to activate that installation. Verify IDF_PATH, Python, CMake, Ninja and xtensa tools match the terminal. Open this root CMake project, select Ninja and the IDF environment; IDF selects the cross compiler, so do not force MSVC/MinGW for firmware.

The included CMake presets provide xinput, hid and host-tests profiles. Select the matching environment for each.

Use build directory `build-clion` and CMake options `-DIDF_TARGET=esp32s3 -DSDKCONFIG=build-clion/sdkconfig`. Reload while online for initial dependency installation. Build target `all`. Flash via the ESP-IDF plugin or an external tool `idf.py -B build-clion -D SDKCONFIG=build-clion/sdkconfig -p COMx flash`. Use the same configuration for terminal and IDE builds.

For debugging use an external JTAG probe, IDF OpenOCD and xtensa-esp-elf-gdb in a CLion OpenOCD configuration, after checking accessible JTAG pads/pins on the board. Native USB-JTAG cannot share the running gamepad's PHY. UART logs and ROM download mode provide recovery.

For host tests create a separate CLion profile with a desktop C++17 compiler and `-DVSLINK_HOST_TESTS=ON`, build directory `build-host`. No board or IDF environment is required:

```powershell
cmake -S . -B build-host -DVSLINK_HOST_TESTS=ON
cmake --build build-host --config Debug
ctest --test-dir build-host -C Debug --output-on-failure
```

## Architecture and recovery

NimBLE callbacks copy complete writes into a static FIFO under a short FreeRTOS critical section. Its configurable depth defaults to 64, maximum command length 512 bytes. The parser/FIFO avoid heap allocation. The main FreeRTOS task alone owns controller state and submits USB reports; BLE callbacks never call USB. Each report is retried until accepted before consuming another command, preserving short press/release edges. Unchanged state refreshes every 20 ms to handle USB remounts.

Disconnect immediately discards queued commands and flags reset outside the bounded queue. The controller task clears all state at its next iteration and retries neutral until USB accepts it, before allowing reconnect input. An already submitted transfer cannot be recalled. Overflow uses the same neutral recovery but retains the selected controller type; the overflowing write is rejected. Subsequent writes may proceed after neutral. Lost events must be reasserted by the app. An overflow counter is available for diagnostics. No inactivity timer interrupts held controls.

BLE requests 7.5-15 ms connection intervals, zero slave latency, 2 s supervision timeout; the central chooses final parameters. No measured latency guarantee is claimed. Pairing is not required. Advertising resumes after disconnect. PS controls translate into the fixed USB identity; this is not a genuine DualShock controller.
