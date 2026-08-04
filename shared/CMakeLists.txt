cmake_minimum_required(VERSION 3.20.0)

# Reusable CMake library targets within the shared workspace for Seeed Studio XIAO nRF52840 Sense Plus.

# Define the shared_imu static library wrapper for the onboard LSM6DS3TR-C IMU.
add_library(shared_imu STATIC drivers/imu/imu_driver.c)
target_include_directories(shared_imu PUBLIC drivers/imu)
target_link_libraries(shared_imu PUBLIC zephyr_interface)

# Define the shared_utils utility library target.
add_library(shared_utils STATIC lib/utils/utils.c)
target_include_directories(shared_utils PUBLIC lib/utils)
target_link_libraries(shared_utils PUBLIC zephyr_interface)
