# Build and test results

Validated locally on Windows on 2026-10-21. No board was flashed and no physical BLE/USB or mobile-app test was performed. Windows XUSB driver binding remains a hardware acceptance check, not a conclusion from compilation.

## Automated results

| Check | Result |
|---|---|
| Desktop C++17 parser/state/queue/report scheduler suite | PASS: 752 checks, CTest 1/1 |
| Every key from upstream Xbox and DS4 action maps | Accepted in the correct mode (fixture extracted from the audited source) |
| XInput firmware compile, link, image generation and partition check | PASS |
| Generic HID firmware compile, link, image generation and partition check | PASS |
| USB mode configuration | XInput selected with HID count 0; HID selected with HID count 1 |
| CMake presets | xinput, hid, host-tests parsed successfully |
| CLion IDE build / remote CI | Not executed; CMake project/presets and CI workflow supplied |
| Flash, Windows enumeration, original mobile app, radio reconnects, timing | Not physically tested; follow INTEGRATION.md |

The host suite covers all button names/aliases and releases, simultaneous inputs, discrete/continuous sticks, centre/reset, trigger 0/0.5/1, clamping, malformed/non-finite/oversized/NUL input, repeated presses/releases, type switching, PS translations, FIFO ordering, overflow recovery, disconnect/reconnect reset, USB backpressure and neutral priority. A busy mock USB verifies that a pending press cannot be overwritten by release, and reset supersedes pending analog state. Idle-refresh tests ensure it does not starve newly queued input.

## Build environment

- ESP-IDF v5.5.1 (`fcae32885b0296b32044cb99ecbdc50d98dddb83`), target esp32s3.
- xtensa-esp-elf GCC 14.2.0 (esp-14.2.0_20241119), CMake 3.30.2, Ninja 1.12.1, Python 3.12.
- ESPP usb_device git revision `792c56e1a5cdf3938868990425d8e45cd0e6ed0c`.
- esp_tinyusb 2.4.0; TinyUSB 0.21.0~2; ESPP base_component/logger/format 1.3.9. Exact provenance/hashes: dependencies.lock.
- Host tests: Visual Studio 2022 MSVC 19.37, C++17, /W4 /WX.
- Compilation database verified C++17 for main, BLE, parser, state and connection modules; USB adapter alone explicitly uses C++20 because ESPP requires std::span. Dependency sources retain their own language settings.

Configured through idf.py with distinct build directories/sdkconfig files and the documented defaults. Actual compilation was completed using `ninja -C build-xinput -j4` and `ninja -C build-hid -j4` in the activated tool environment. Four jobs avoided excessive compiler concurrency on this Windows host. Host tests were built through CMake/NMake and run with CTest.

## Artifacts

Both images fit the default 0x100000-byte factory app partition. Bootloader size is 0x5240, within its limit.

- `xinput`: `build-xinput/vslink_receiver.bin`, 941,584 bytes, SHA-256 `01dceb2df68175ca5b5a92fac88222ad9e2cfbc8a72b1829d583ba33508c6fd7`.
- `hid`: `build-hid/vslink_receiver.bin`, 945,408 bytes, SHA-256 `77f47e54a994a5d2958a4f5cdd307a40d5d2724c8fdea9d09f51470cc445d9e6`.

Each build directory also contains `vslink_receiver.elf`, `bootloader/bootloader.bin`, `partition_table/partition-table.bin` and `flasher_args.json`. Flash with idf.py using the matching build directory/configuration, rather than writing the app image at offset zero. Generated build directories and local reference/toolchain downloads are ignored by Git.

## Compatibility fixes discovered during validation

The unrestricted upstream manifest resolved esp_tinyusb 3.0.0, which bundles TinyUSB and conflicted with the separate 0.21 component: its duplicate build lacked tusb_config.h. Constraining esp_tinyusb to 2.x resolved this without modifying dependency source or replacing XInput. The final lockfile records 2.4.0.

The reference float API uses round(value * 32767) for both signs, not asymmetric -32768 scaling. The implementation matches that behavior with ties-to-even rounding. The NimBLE address utility requires host/util/util.h on the chosen IDF version; the compiled implementation includes it.

A firmware restricted entirely to C++17 is not supplied: the documented C++20 USB adapter exception is required by the selected maintained ESPP API. Hardware acceptance is still required before claiming end-to-end compatibility, reliable Windows enumeration or measured gaming latency.

## Physical flashing update

The user's ESP32-S3 (QFN56, revision v0.2) was detected on COM5 (USB 303A:1001) and flashed successfully using the XInput build. esptool verified the bootloader, application and partition-table hashes. Immediately following automatic reset, Windows still exposed USB Serial/JTAG rather than 045E:028E. A manual board reset and gamepad enumeration/input tests remain pending. This confirms successful physical flashing, not successful controller operation.
