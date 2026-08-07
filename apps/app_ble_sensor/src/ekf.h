#ifndef EKF_H
#define EKF_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Structure holding IMU payload values to be streamed over BLE.
 */
typedef struct __attribute__((packed)) {
    float roll;     // Degrees
    float pitch;    // Degrees
    float yaw;      // Degrees
    float accel_x;  // m/s^2
    float accel_y;  // m/s^2
    float accel_z;  // m/s^2
    float gyro_x;   // rad/s
    float gyro_y;   // rad/s
    float gyro_z;   // rad/s
} imu_ble_payload_t;

/**
 * @brief Initialize the Extended Kalman / Quaternion Filter module.
 */
void ekf_init(void);

/**
 * @brief Update orientation state with raw accelerometer (m/s^2) and gyroscope (rad/s) measurements.
 * 
 * @param ax Accel X (m/s^2)
 * @param ay Accel Y (m/s^2)
 * @param az Accel Z (m/s^2)
 * @param gx Gyro X (rad/s)
 * @param gy Gyro Y (rad/s)
 * @param gz Gyro Z (rad/s)
 * @param dt Sampling period (seconds)
 */
void ekf_update(float ax, float ay, float az, float gx, float gy, float gz, float dt);

/**
 * @brief Get the fused orientation angles in degrees.
 * 
 * @param roll Pointer to store roll angle (-180 to +180 deg)
 * @param pitch Pointer to store pitch angle (-90 to +90 deg)
 * @param yaw Pointer to store yaw angle (-180 to +180 deg)
 */
void ekf_get_angles(float *roll, float *pitch, float *yaw);

/**
 * @brief Start a gyroscope calibration routine (requires board to be stationary).
 */
void ekf_start_calibration(void);

/**
 * @brief Check if gyro bias calibration is currently active.
 * 
 * @return true if calibrating, false otherwise.
 */
bool ekf_is_calibrating(void);

/**
 * @brief Process a gyro measurement sample during calibration.
 * 
 * @param gx Gyro X (rad/s)
 * @param gy Gyro Y (rad/s)
 * @param gz Gyro Z (rad/s)
 * @return true when calibration completes successfully, false if still gathering samples.
 */
bool ekf_update_calibration(float gx, float gy, float gz);

#endif // EKF_H
