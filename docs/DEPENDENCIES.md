# Dependency provenance

| Dependency | Source / license |
|---|---|
| VSLink protocol and action names | https://github.com/jackjburnett/VSLink at 8d22865985a017b6bf31500f7dffb83edd604c8f; upstream GPL-3.0 |
| ESPP usb_device | https://github.com/esp-cpp/espp at 792c56e1a5cdf3938868990425d8e45cd0e6ed0c; component MIT |
| ESP-IDF | https://github.com/espressif/esp-idf tag v5.5.1; upstream licenses apply |
| esp_tinyusb / TinyUSB / ESPP support components | Exact resolved versions and hashes in dependencies.lock; packaged upstream licenses apply |

The protocol test fixture records the command keys from both upstream action maps. Reference clones and toolchain downloads used during development are ignored by Git and are not required by host tests or managed builds. The firmware uses ESPP's maintained USB driver rather than copying/reimplementing XInput. Consult the fetched dependencies' own license files when redistributing them.
