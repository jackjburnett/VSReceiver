# Manual integration tests

Hardware tests have not been performed in this environment. Record board, Windows version, phone/app version, firmware mode and observations.

1. Build and flash XInput firmware to the XIAO ESP32-S3. Use BOOT+RESET to enter native USB ROM download mode, flash, then reset.
2. Connect its native USB-C data port to Windows. In Device Manager confirm an Xbox 360-compatible controller with the Windows XUSB driver. Enumeration sounds alone do not prove driver binding. The Windows VSLink receiver and ViGEmBus are not needed.
3. Run `joy.cpl`, select the controller, open Properties. Also test an XInput game/API tester to confirm actual XInput binding; Guide may not appear in joy.cpl.
4. Connect the original VSLink-compatible mobile app using BLE and the existing service UUID. Do not change its packets. Close other BLE central/scanner apps. Browser-based clients require that app's supported Web Bluetooth browser/environment and permissions.
5. Verify every face button, bumper, D-pad direction, Start/Back, stick click and Guide. Test A+B+LB together, repeated presses, releases and release without prior press.
6. Sweep both analog sticks independently and together. Verify X right/Y up, full range, centre, discrete joystick directions and reset.
7. Sweep both triggers independently through 0, 0.5, 1 and together. joy.cpl may combine trigger display; use an XInput tester to inspect independent LT/RT values.
8. Hold several buttons, a stick and both triggers; disconnect BLE and confirm everything returns to neutral. Repeat with phone Bluetooth disabled and radio loss; link-loss cleanup follows the supervision timeout, unlike explicit disconnect.
9. Reconnect repeatedly. Verify advertising returns, controls work, and no stale analog values or controller type remain.
10. Switch app to PS and verify translated face buttons, L1/R1, L2/R2, Share/Options/L3/R3/PS and Touchpad-click approximation. Each type switch must clear holds without USB re-enumeration.
11. Stress with rapid button/stick traffic. Using a diagnostic BLE central, exceed queue capacity deliberately: backlog must be discarded, neutral sent, then new inputs accepted. Confirm no persistent stuck buttons. Lost events are not replayed.
12. If safely powered independently, unplug/reconnect USB while BLE stays connected; verify current state refreshes after mount. Avoid unsafe power backfeeding.
13. Build/flash HID using a separate sdkconfig. Confirm only a HID Game Pad interface, four stick axes, two trigger axes and buttons including the four D-pad buttons. Repeat disconnect/reconnect tests. HID is not XInput.

Use USBView or a USB analyzer to inspect descriptors. Measure end-to-end latency under load before making latency claims. Neither requested BLE intervals nor USB polling intervals establish actual latency.
