cmake_minimum_required(VERSION 3.20.0)

# Locate the Zephyr package to configure compiler flags, system libraries and includes.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_template)

# Pull in the shared libraries by adding the subdirectory.
# CMAKE_CURRENT_BINARY_DIR/shared ensures binary targets are generated separately.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../shared ${CMAKE_CURRENT_BINARY_DIR}/shared)

# Add application sources
target_sources(app PRIVATE src/main.c)

# Link the shared driver and utility libraries to the main application target
target_link_libraries(app PRIVATE
    shared_imu
    shared_utils
    shared_hal
    shared_uart
    shared_spi
    shared_i2c
    shared_i2s
    shared_ble
    shared_usb_cdc
    shared_nfc
    shared_rgb_led
    shared_battery
    shared_button
    shared_microphone
)
