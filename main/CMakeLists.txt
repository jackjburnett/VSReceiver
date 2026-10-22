idf_component_register(SRCS main.cpp BleReceiver.cpp VslinkParser.cpp GamepadStateManager.cpp UsbGamepad.cpp ConnectionManager.cpp
  INCLUDE_DIRS "." REQUIRES bt nvs_flash usb_device)
# ESPP's public headers use std::span; keep portable protocol files C++17.
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
set_source_files_properties(main.cpp BleReceiver.cpp ConnectionManager.cpp VslinkParser.cpp GamepadStateManager.cpp PROPERTIES COMPILE_FLAGS "-std=gnu++17")
set_source_files_properties(UsbGamepad.cpp PROPERTIES COMPILE_FLAGS "-std=gnu++20")
