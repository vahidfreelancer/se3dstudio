#include "ekf.h"
#include <math.h>
#include <string.h>

#define M_PI_F 3.14159265358979323846f

/* Quaternion state vector */
static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;

/* Gyroscope biases (rad/s) */
static float gyro_bias_x = 0.0f;
static float gyro_bias_y = 0.0f;
static float gyro_bias_z = 0.0f;

/* Filter gains */
static const float Kp = 1.0f; // Proportional gain for accel feedback
static const float Ki = 0.005f; // Integral gain

static float eIntX = 0.0f, eIntY = 0.0f, eIntZ = 0.0f;

/* Calibration state */
static bool calibrating = false;
static uint16_t cal_samples = 0;
#define CAL_TOTAL_SAMPLES 90 // ~3 seconds at 30Hz

static float cal_sum_gx = 0.0f;
static float cal_sum_gy = 0.0f;
static float cal_sum_gz = 0.0f;

void ekf_init(void)
{
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
    
    eIntX = 0.0f;
    eIntY = 0.0f;
    eIntZ = 0.0f;

    gyro_bias_x = 0.0f;
    gyro_bias_y = 0.0f;
    gyro_bias_z = 0.0f;
    
    calibrating = false;
    cal_samples = 0;
}

void ekf_start_calibration(void)
{
    calibrating = true;
    cal_samples = 0;
    cal_sum_gx = 0.0f;
    cal_sum_gy = 0.0f;
    cal_sum_gz = 0.0f;
}

bool ekf_is_calibrating(void)
{
    return calibrating;
}

bool ekf_update_calibration(float gx, float gy, float gz)
{
    if (!calibrating) {
        return false;
    }

    cal_sum_gx += gx;
    cal_sum_gy += gy;
    cal_sum_gz += gz;
    cal_samples++;

    if (cal_samples >= CAL_TOTAL_SAMPLES) {
        gyro_bias_x = cal_sum_gx / (float)CAL_TOTAL_SAMPLES;
        gyro_bias_y = cal_sum_gy / (float)CAL_TOTAL_SAMPLES;
        gyro_bias_z = cal_sum_gz / (float)CAL_TOTAL_SAMPLES;
        
        calibrating = false;
        return true; // Completed
    }

    return false;
}

void ekf_update(float ax, float ay, float az, float gx, float gy, float gz, float dt)
{
    // Subtract bias offset
    gx -= gyro_bias_x;
    gy -= gyro_bias_y;
    gz -= gyro_bias_z;

    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm > 0.0001f) {
        ax /= norm;
        ay /= norm;
        az /= norm;

        // Estimated direction of gravity from quaternion state
        float vx = 2.0f * (q1 * q3 - q0 * q2);
        float vy = 2.0f * (q0 * q1 + q2 * q3);
        float vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

        // Cross product error between estimated gravity and accelerometer vector
        float ex = (ay * vz - az * vy);
        float ey = (az * vx - ax * vz);
        float ez = (ax * vy - ay * vx);

        // Integral error
        if (Ki > 0.0f) {
            eIntX += ex * dt;
            eIntY += ey * dt;
            eIntZ += ez * dt;
        } else {
            eIntX = 0.0f;
            eIntY = 0.0f;
            eIntZ = 0.0f;
        }

        // Apply feedback to gyro measurements
        gx += Kp * ex + Ki * eIntX;
        gy += Kp * ey + Ki * eIntY;
        gz += Kp * ez + Ki * eIntZ;
    }

    // Integrate quaternion rate of change
    float q0_dot = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    float q1_dot = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    float q2_dot = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    float q3_dot = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    q0 += q0_dot * dt;
    q1 += q1_dot * dt;
    q2 += q2_dot * dt;
    q3 += q3_dot * dt;

    // Normalize quaternion
    norm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm > 0.0001f) {
        q0 /= norm;
        q1 /= norm;
        q2 /= norm;
        q3 /= norm;
    }
}

void ekf_get_angles(float *roll, float *pitch, float *yaw)
{
    if (roll) {
        *roll = atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) * (180.0f / M_PI_F);
    }
    if (pitch) {
        float sinp = 2.0f * (q0 * q2 - q3 * q1);
        if (fabsf(sinp) >= 1.0f) {
            *pitch = copysignf(90.0f, sinp);
        } else {
            *pitch = asinf(sinp) * (180.0f / M_PI_F);
        }
    }
    if (yaw) {
        *yaw = atan2f(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3)) * (180.0f / M_PI_F);
    }
}
