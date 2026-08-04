cmake_minimum_required(VERSION 3.20.0)

# Reusable CMake library targets within the shared workspace for Seeed Studio XIAO nRF52840 Sense Plus.

# Define the shared_imu static library wrapper for the onboard LSM6DS3TR-C IMU.
add_library(shared_imu STATIC drivers/imu/imu_driver.c)
target_include_directories(shared_imu PUBLIC drivers/imu)
target_link_libraries(shared_imu PUBLIC zephyr_interface)
add_dependencies(shared_imu zephyr_generated_headers)

# Define the shared_utils utility library target.
add_library(shared_utils STATIC lib/utils/utils.c)
target_include_directories(shared_utils PUBLIC lib/utils)
target_link_libraries(shared_utils PUBLIC zephyr_interface)
add_dependencies(shared_utils zephyr_generated_headers)

# Define the shared_hal static library wrapping core hardware peripherals.
add_library(shared_hal STATIC
    hal/gpio/hal_gpio.c
    hal/adc/hal_adc.c
    hal/pwm/hal_pwm.c
    hal/timer/hal_timer.c
    hal/rtc/hal_rtc.c
    hal/watchdog/hal_watchdog.c
    hal/flash/hal_flash.c
    hal/rng/hal_rng.c
    hal/system/hal_system.c
    hal/power/hal_power.c
)

target_include_directories(shared_hal PUBLIC
    hal/gpio
    hal/adc
    hal/pwm
    hal/timer
    hal/rtc
    hal/watchdog
    hal/flash
    hal/rng
    hal/system
    hal/power
)

target_link_libraries(shared_hal PUBLIC zephyr_interface)
add_dependencies(shared_hal zephyr_generated_headers)

