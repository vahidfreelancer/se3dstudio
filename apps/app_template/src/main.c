#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <stdio.h>
#include <string.h>

/* Include core HAL libraries */
#include "hal_pwm.h"
#include "hal_system.h"

/* Include BLE Driver */
#include "ble_driver.h"

/* Register application log module */
LOG_MODULE_REGISTER(app_pwm_led, LOG_LEVEL_INF);

/* Get PWM specifications from Devicetree aliases */
static const struct pwm_dt_spec pwm_r = PWM_DT_SPEC_GET(DT_ALIAS(pwm_red));
static const struct pwm_dt_spec pwm_g = PWM_DT_SPEC_GET(DT_ALIAS(pwm_green));
static const struct pwm_dt_spec pwm_b = PWM_DT_SPEC_GET(DT_ALIAS(pwm_blue));

/* Structure for representing RGB values */
struct color_rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    const char *name;
};

/* 12 Major Color Wheel Colors */
static const struct color_rgb colors_12[12] = {
    {255, 0, 0, "Red"},
    {255, 127, 0, "Orange"},
    {255, 255, 0, "Yellow"},
    {127, 255, 0, "Chartreuse"},
    {0, 255, 0, "Green"},
    {0, 255, 127, "Spring Green"},
    {0, 255, 255, "Cyan"},
    {0, 127, 255, "Azure"},
    {0, 0, 255, "Blue"},
    {127, 0, 255, "Violet"},
    {255, 0, 255, "Magenta"},
    {255, 0, 127, "Rose"}
};

/**
 * @brief Sets the RGB LED color by translating 0-255 brightness to PWM pulse widths.
 */
static int set_led_color(const struct color_rgb *color)
{
    int ret;
    
    // Scale 0-255 values to nanoseconds based on period
    uint32_t r_pulse = (color->r * pwm_r.period) / 255;
    uint32_t g_pulse = (color->g * pwm_g.period) / 255;
    uint32_t b_pulse = (color->b * pwm_b.period) / 255;

    ret = hal_pwm_set_period_and_duty(&pwm_r, pwm_r.period, r_pulse);
    if (ret < 0) return ret;

    ret = hal_pwm_set_period_and_duty(&pwm_g, pwm_g.period, g_pulse);
    if (ret < 0) return ret;

    ret = hal_pwm_set_period_and_duty(&pwm_b, pwm_b.period, b_pulse);
    if (ret < 0) return ret;

    return 0;
}

int main(void)
{
    int ret;
    char ble_name[32];
    uint8_t uuid[8];

    LOG_INF("Starting 12-Color PWM RGB LED Cycle & Custom BLE Name Application...");

    /* Log System Hardware Details */
    hal_system_log_info();

    /* Check if PWM devices are ready */
    if (!device_is_ready(pwm_r.dev) || !device_is_ready(pwm_g.dev) || !device_is_ready(pwm_b.dev)) {
        LOG_ERR("PWM devices for RGB LED are not ready");
        return -ENODEV;
    }

    /* Initialize BLE Stack */
    ret = ble_drv_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize BLE (err %d)", ret);
        return ret;
    }

    /* Retrieve Device Unique ID to generate a 4-digit hexadecimal suffix */
    int len = hal_system_get_uuid(uuid, sizeof(uuid));
    if (len == 8) {
        // Use the lower 16 bits of the 64-bit unique ID
        uint16_t dev_id = ((uint16_t)uuid[1] << 8) | uuid[0];
        snprintf(ble_name, sizeof(ble_name), "se3dstudio_%04X", dev_id);
    } else {
        snprintf(ble_name, sizeof(ble_name), "se3dstudio_0000");
    }

    /* Start advertising with dynamic device name */
    ret = ble_drv_adv_start(ble_name);
    if (ret < 0) {
        LOG_ERR("Failed to start BLE advertising (err %d)", ret);
        return ret;
    }

    LOG_INF("BLE advertising started as '%s'. Initiating color cycle...", ble_name);

    int color_idx = 0;
    while (1) {
        const struct color_rgb *color = &colors_12[color_idx];
        
        ret = set_led_color(color);
        if (ret < 0) {
            LOG_ERR("Failed to set color (err %d)", ret);
        } else {
            LOG_INF("Switching to Color [%2d/12]: %-15s (R:%3d, G:%3d, B:%3d)",
                    color_idx + 1, color->name, color->r, color->g, color->b);
        }

        // Notify connected Bluetooth clients of the current color index
        uint8_t notify_data[1] = { (uint8_t)color_idx };
        ble_drv_notify(notify_data, sizeof(notify_data));

        color_idx = (color_idx + 1) % 12;
        k_msleep(200);
    }

    return 0;
}
