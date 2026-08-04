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

# Define communication libraries
add_library(shared_uart STATIC drivers/uart/uart_driver.c)
target_include_directories(shared_uart PUBLIC drivers/uart)
target_link_libraries(shared_uart PUBLIC zephyr_interface)
add_dependencies(shared_uart zephyr_generated_headers)

add_library(shared_spi STATIC drivers/spi/spi_driver.c)
target_include_directories(shared_spi PUBLIC drivers/spi)
target_link_libraries(shared_spi PUBLIC zephyr_interface)
add_dependencies(shared_spi zephyr_generated_headers)

add_library(shared_i2c STATIC drivers/i2c/i2c_driver.c)
target_include_directories(shared_i2c PUBLIC drivers/i2c)
target_link_libraries(shared_i2c PUBLIC zephyr_interface)
add_dependencies(shared_i2c zephyr_generated_headers)

add_library(shared_i2s STATIC drivers/i2s/i2s_driver.c)
target_include_directories(shared_i2s PUBLIC drivers/i2s)
target_link_libraries(shared_i2s PUBLIC zephyr_interface)
add_dependencies(shared_i2s zephyr_generated_headers)

add_library(shared_ble STATIC drivers/ble/ble_driver.c)
target_include_directories(shared_ble PUBLIC drivers/ble)
target_link_libraries(shared_ble PUBLIC zephyr_interface)
add_dependencies(shared_ble zephyr_generated_headers)

add_library(shared_usb_cdc STATIC drivers/usb_cdc/usb_cdc_driver.c)
target_include_directories(shared_usb_cdc PUBLIC drivers/usb_cdc)
target_link_libraries(shared_usb_cdc PUBLIC zephyr_interface)
add_dependencies(shared_usb_cdc zephyr_generated_headers)

add_library(shared_nfc STATIC drivers/nfc/nfc_driver.c)
target_include_directories(shared_nfc PUBLIC drivers/nfc)
target_link_libraries(shared_nfc PUBLIC zephyr_interface)
add_dependencies(shared_nfc zephyr_generated_headers)

# Define board-specific libraries
add_library(shared_rgb_led STATIC drivers/rgb_led/rgb_led_driver.c)
target_include_directories(shared_rgb_led PUBLIC drivers/rgb_led)
target_link_libraries(shared_rgb_led PUBLIC zephyr_interface)
add_dependencies(shared_rgb_led zephyr_generated_headers)

add_library(shared_battery STATIC drivers/battery/battery_driver.c)
target_include_directories(shared_battery PUBLIC drivers/battery)
target_link_libraries(shared_battery PUBLIC zephyr_interface)
add_dependencies(shared_battery zephyr_generated_headers)

add_library(shared_button STATIC drivers/button/button_driver.c)
target_include_directories(shared_button PUBLIC drivers/button)
target_link_libraries(shared_button PUBLIC zephyr_interface)
add_dependencies(shared_button zephyr_generated_headers)

add_library(shared_microphone STATIC drivers/microphone/mic_driver.c)
target_include_directories(shared_microphone PUBLIC drivers/microphone)
target_link_libraries(shared_microphone PUBLIC zephyr_interface)
add_dependencies(shared_microphone zephyr_generated_headers)

