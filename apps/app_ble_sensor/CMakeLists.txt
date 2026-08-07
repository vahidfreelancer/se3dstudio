cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_ble_sensor)

# Add the shared library subdirectory so CMake builds it first
add_subdirectory(../../shared ../../shared)

# Include shared headers
target_include_directories(app PRIVATE
    ../../shared/include
    ../../shared/hal/gpio
    ../../shared/hal/adc
    ../../shared/hal/pwm
    ../../shared/hal/timer
    ../../shared/hal/rtc
    ../../shared/hal/watchdog
    ../../shared/hal/flash
    ../../shared/hal/rng
    ../../shared/hal/system
    ../../shared/hal/power
    ../../shared/drivers/uart
    ../../shared/drivers/spi
    ../../shared/drivers/i2c
    ../../shared/drivers/i2s
    ../../shared/drivers/imu
    ../../shared/drivers/ble
    ../../shared/drivers/usb_cdc
    ../../shared/drivers/nfc
    ../../shared/drivers/rgb_led
    ../../shared/drivers/battery
    ../../shared/drivers/button
    ../../shared/drivers/microphone
    src
)

target_sources(app PRIVATE
    src/main.c
    src/ekf.c
)

# Link necessary driver libraries
target_link_libraries(app PRIVATE
    shared_utils
    shared_hal
    shared_imu
    shared_ble
    shared_rgb_led
    shared_microphone
)
