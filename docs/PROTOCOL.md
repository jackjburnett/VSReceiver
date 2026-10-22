# VSLink protocol audit and mapping

Inspected VSLink commit `8d22865985a017b6bf31500f7dffb83edd604c8f`: BLE.py, BluetoothManager.py, GamepadParser.py, GamepadManager.py, ControllerRouter.py, and ACTION_MAP_XBOX/JOYSTICK/TRIGGERS/DS4.py. Analog conversion was checked against its vgamepad 0.1.0 dependency. ESPP usb_device headers, source, manifest and xinput_example were inspected at commit `792c56e1a5cdf3938868990425d8e45cd0e6ed0c`.

## Transport

| Property | Value |
|---|---|
| Service | `d749b3ea-d666-4f2f-b49d-3e43fc6db631` |
| Input characteristic | `d749b3ea-d666-4f2f-b49d-3e43fc6db632` |
| Properties | Write, Write Without Response |
| Security | Plain writes; no pairing, encryption or bonding requirement |
| Message | One UTF-8 command per write, 1-512 bytes |
| Advertising | Connectable/discoverable; service UUID advertised; VSLink name in scan response |

The Windows source leaves name/address advertising details to the OS; discovery should use its service UUID. No new framing, batching, notifications or acknowledgement messages exist. Here a connection initializes an Xbox controller; upstream allocates on first valid write, which has no visible input effect.

All command words are ASCII, a valid UTF-8 subset. Leading/trailing ASCII whitespace is stripped. Commands are case sensitive except the controller type value. Empty/oversized messages, NUL, non-ASCII, non-finite analog numbers, malformed numbers and unknown commands do not change state. Unicode whitespace and Python-only numeric digit separators are not supported. Standard signed decimal/exponent forms are supported. Normal commands fit in a negotiated ATT write; do not split a command across ordinary writes. MTU preference is 517; longer commands need a suitable negotiated MTU or GATT long-write transaction.

## Xbox buttons

All names below accept `_PRESS` and `_RELEASE`.

| Actual source names | Output (button bit) |
|---|---|
| A, B, X, Y | A (12), B (13), X (14), Y (15) |
| LEFT_BUMPER, LB | Left bumper (8) |
| RIGHT_BUMPER, RB | Right bumper (9) |
| BACK, START | Back (5), Start (4) |
| LEFT_THUMB, LS | Left stick click (6) |
| RIGHT_THUMB, RS | Right stick click (7) |
| HOME, GUIDE, XBOX_BUTTON | Guide (10) |
| DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT | Independent bits 0, 1, 2, 3 |
| LEFT_TRIGGER, LT | Left trigger 255 / 0 |
| RIGHT_TRIGGER, RT | Right trigger 255 / 0 |

The source's test examples use LEFT_SHOULDER/RIGHT_SHOULDER, although its actual Xbox action map uses LEFT_BUMPER/RIGHT_BUMPER. The firmware explicitly accepts the two SHOULDER spellings requested by the task as compatibility aliases. The header's LEFT_BUMBER typo is not a supported map key. No incoming commands are renamed. Aliases share bits as in upstream: they are not counted as independent physical sources.

Repeated presses/releases are idempotent. A button update preserves all unrelated bits. Xbox opposite D-pad directions remain independently set if sent together.

## Sticks / triggers

`LEFT_JOYSTICK_<x>_<y>` and `RIGHT_JOYSTICK_<x>_<y>` clamp coordinates independently to [-1,1]. Xbox X right and Y up are positive. No dead zone or circular normalization. Output is Python ties-to-even **round(value * 32767)**. Thus -1=-32767, 0=0, 0.5=16384, 1=32767; the signed 16-bit field supports -32768 but the reference float conversion does not produce it.

Both sticks also accept suffixes `_LEFT`, `_RIGHT`, `_UP`, `_DOWN`, `_RESET`. A direction sets one axis fully and the other to zero; RESET centres that stick. `RESET` alone clears the entire gamepad without changing type.

Continuous triggers use `<name>_<value>` with names LEFT_TRIGGER, LT, L2, RIGHT_TRIGGER, RT, R2 in either mode. Clamp to [0,1], then ties-to-even **round(value * 255)**: 0=0, 0.5=128, 1=255. Left and right values are independent. Digital trigger aliases depend on selected type.

## Controller type / PS translation

The exact prefix `Change gamepad type to:` accepts a trimmed, case-insensitive XBOX or PS. No space after the colon is required. Upstream destroys and recreates the virtual device, releasing inputs; here state resets and the USB identity stays fixed. Reconnection starts in Xbox mode.

PS names accept `_PRESS` and `_RELEASE`:

| PS name | Fixed USB equivalent |
|---|---|
| CROSS, CIRCLE, SQUARE, TRIANGLE | A, B, X, Y |
| L1, R1 | LB, RB |
| SHARE, OPTIONS | Back, Start |
| L3, R3 | Stick clicks |
| PS_BUTTON | Guide |
| TOUCHPAD | Back (click approximation, shares bit with SHARE) |
| L2, R2 | Left/right trigger full/zero |
| DPAD_UP/DOWN/LEFT/RIGHT | Cardinal D-pad |

Like the DS4 source map, a PS D-pad press selects one cardinal and any D-pad release clears the hat; unrelated buttons remain held. The Xbox and PS button maps are exclusive. Shared continuous trigger aliases work in both modes.

The DS4 discrete UP source is Y=-1 and DOWN is +1, translated here to Xbox up-positive coordinates. Continuous PS Y values are inverted for the same reason. Translation retains signed 16-bit precision instead of quantizing through DS4's 8-bit stick reports. No genuine DS4 identity, touch positions, motion, audio or authentication is implemented. USB rumble/LED output has no physical effect.

## Generic HID

One standard HID Game Pad application collection, no report ID. A 12-byte input report contains 16 buttons in Xbox bit positions (D-pad = buttons 1-4), four signed 16-bit axes X/Y/Rx/Ry, and two unsigned 8-bit trigger axes Z/Rz. Reserved Xbox bit 11 stays unused. HID Y axes are down-positive. Poll interval is 4 ms. D-pad uses independent buttons rather than a hat, preserving simultaneous Xbox direction bits. Games may need manual mapping; HID mode is not XInput.
