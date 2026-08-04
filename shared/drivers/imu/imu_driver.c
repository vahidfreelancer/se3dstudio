#include "imu_driver.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifndef CONFIG_IMU_DRIVER_LOG_LEVEL
#define CONFIG_IMU_DRIVER_LOG_LEVEL LOG_LEVEL_INF
#endif

LOG_MODULE_REGISTER(imu_driver, CONFIG_IMU_DRIVER_LOG_LEVEL);

/* The LSM6DS3TR-C on Seeed Studio XIAO nRF52840 Sense Plus maps to compatible "st,lsm6dsl" */
#define IMU_COMPAT st_lsm6dsl

#if DT_HAS_COMPAT_STATUS_OKAY(IMU_COMPAT)
#define IMU_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(IMU_COMPAT)
static const struct device *const imu_dev = DEVICE_DT_GET(IMU_NODE);
#else
#error "LSM6DSL (compatible st,lsm6dsl) node not found or disabled in devicetree!"
static const struct device *const imu_dev = NULL;
#endif

int imu_driver_init(void)
{
    if (imu_dev == NULL) {
        LOG_ERR("IMU device pointer is NULL");
        return -ENODEV;
    }

    if (!device_is_ready(imu_dev)) {
        LOG_ERR("IMU device %s is not ready", imu_dev->name);
        return -ENODEV;
    }

    LOG_INF("IMU device %s successfully initialized", imu_dev->name);
    return 0;
}

int imu_driver_fetch(struct imu_data *data)
{
    if (data == NULL) {
        return -EINVAL;
    }

    if (imu_dev == NULL || !device_is_ready(imu_dev)) {
        return -ENODEV;
    }

    int ret = sensor_sample_fetch(imu_dev);
    if (ret < 0) {
        LOG_ERR("Failed to fetch sensor sample (err %d)", ret);
        return ret;
    }

    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, data->accel);
    if (ret < 0) {
        LOG_ERR("Failed to get accelerometer channel (err %d)", ret);
        return ret;
    }

    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, data->gyro);
    if (ret < 0) {
        LOG_ERR("Failed to get gyroscope channel (err %d)", ret);
        return ret;
    }

    return 0;
}
