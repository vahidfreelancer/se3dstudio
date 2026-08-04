#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

/* Include shared libraries */
#include "imu_driver.h"
#include "utils.h"

/* Register application log module */
LOG_MODULE_REGISTER(app_template, LOG_LEVEL_INF);

/* Get the LED0 devicetree node alias (typically Red LED on Seeed Studio XIAO boards) */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_EXISTS(LED0_NODE)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#else
#warning "led0 alias is not defined in the devicetree. LED toggling will be disabled!"
#endif

int main(void)
{
    int ret;

    LOG_INF("Starting Application Template on Seeed Studio XIAO nRF52840 Sense Plus...");

    /* Initialize user LED if configured in the devicetree */
#if DT_NODE_EXISTS(LED0_NODE)
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED GPIO device is not ready!");
    } else {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED pin (err %d)", ret);
        } else {
            LOG_INF("LED configured successfully");
        }
    }
#endif

    /* Initialize shared IMU driver wrapper */
    ret = imu_driver_init();
    if (ret < 0) {
        LOG_WRN("Failed to initialize IMU driver (err %d). Running in degraded mode.", ret);
    } else {
        LOG_INF("IMU driver initialized successfully");
    }

    struct imu_data imu_readings;

    while (1) {
        /* Toggle user LED if available and ready */
#if DT_NODE_EXISTS(LED0_NODE)
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_toggle_dt(&led);
        }
#endif

        /* Log system status using the shared utility helper */
        utils_log_system_status();

        /* If IMU driver initialized successfully, fetch and print telemetry data */
        if (ret == 0) {
            int fetch_ret = imu_driver_fetch(&imu_readings);
            if (fetch_ret == 0) {
                LOG_INF("IMU Accel: X: %.3f, Y: %.3f, Z: %.3f m/s^2",
                        sensor_value_to_double(&imu_readings.accel[0]),
                        sensor_value_to_double(&imu_readings.accel[1]),
                        sensor_value_to_double(&imu_readings.accel[2]));
                LOG_INF("IMU Gyro:  X: %.3f, Y: %.3f, Z: %.3f rad/s",
                        sensor_value_to_double(&imu_readings.gyro[0]),
                        sensor_value_to_double(&imu_readings.gyro[1]),
                        sensor_value_to_double(&imu_readings.gyro[2]));
            } else {
                LOG_ERR("Failed to fetch data from IMU (err %d)", fetch_ret);
            }
        }

        k_msleep(1000);
    }

    return 0;
}
