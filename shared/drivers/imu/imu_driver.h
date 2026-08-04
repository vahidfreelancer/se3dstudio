#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * @brief Structure to hold IMU readings (acceleration and angular velocity).
 */
struct imu_data {
    struct sensor_value accel[3]; // X, Y, Z acceleration in m/s^2
    struct sensor_value gyro[3];  // X, Y, Z angular velocity in rad/s
};

/**
 * @brief Initialize the onboard LSM6DS3TR-C IMU.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int imu_driver_init(void);

/**
 * @brief Fetch the latest accelerometer and gyroscope data from the IMU.
 * 
 * @param data Pointer to store the fetched IMU data.
 * @return int 0 on success, negative errno on failure.
 */
int imu_driver_fetch(struct imu_data *data);

#endif // IMU_DRIVER_H
