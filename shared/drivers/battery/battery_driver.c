#include "battery_driver.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_ADC_NRFX_SAADC)
#include <hal/nrf_saadc.h>
#endif

LOG_MODULE_REGISTER(battery_driver, LOG_LEVEL_INF);

#define VBATT_DIV_EN_PIN 14
#define CHARGE_STAT_PIN  17

static const struct device *gpio0_dev = NULL;
static const struct device *adc_dev = NULL;

int battery_drv_init(void)
{
    int ret;

    gpio0_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0));
    if (gpio0_dev == NULL || !device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 device not ready for battery driver");
        return -ENODEV;
    }

    adc_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(adc));
    if (adc_dev == NULL || !device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready for battery driver");
        return -ENODEV;
    }

    // Configure battery divider enable pin (output, default HIGH to keep off)
    ret = gpio_pin_configure(gpio0_dev, VBATT_DIV_EN_PIN, GPIO_OUTPUT_HIGH);
    if (ret < 0) {
        LOG_ERR("Failed to configure battery divider pin (err %d)", ret);
        return ret;
    }

    // Configure charging status pin (input with pullup)
    ret = gpio_pin_configure(gpio0_dev, CHARGE_STAT_PIN, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        LOG_ERR("Failed to configure charging status pin (err %d)", ret);
        return ret;
    }

    LOG_INF("Battery driver successfully initialized");
    return 0;
}

int battery_drv_divider_enable(bool enable)
{
    if (gpio0_dev == NULL) {
        return -EINVAL;
    }
    // Pull low (0) to turn on the gating transistor, high (1) to turn off
    return gpio_pin_set(gpio0_dev, VBATT_DIV_EN_PIN, enable ? 0 : 1);
}

int battery_drv_read_voltage(uint32_t *voltage_mv)
{
    int ret;
    int16_t raw_val;

    if (gpio0_dev == NULL || adc_dev == NULL || voltage_mv == NULL) {
        return -EINVAL;
    }

    // Enable the voltage divider
    ret = battery_drv_divider_enable(true);
    if (ret < 0) return ret;

    // Wait for the voltage level to stabilize
    k_busy_wait(10);

    // Set up SAADC channel configuration for AIN7 (P0.31)
    struct adc_channel_cfg ch_cfg = {
        .gain = ADC_GAIN_1_6,
        .reference = ADC_REF_INTERNAL, // 0.6V internal reference on nRF52
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id = 7,
#if defined(CONFIG_ADC_NRFX_SAADC)
        .input_positive = NRF_SAADC_INPUT_AIN7,
#endif
    };

    ret = adc_channel_setup(adc_dev, &ch_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to setup ADC channel for battery (err %d)", ret);
        battery_drv_divider_enable(false);
        return ret;
    }

    struct adc_sequence seq = {
        .channels = BIT(7),
        .buffer = &raw_val,
        .buffer_size = sizeof(raw_val),
        .resolution = 12, // 12-bit SAADC resolution
    };

    ret = adc_read(adc_dev, &seq);
    
    // Disable divider immediately to save power
    battery_drv_divider_enable(false);

    if (ret < 0) {
        LOG_ERR("Failed to read ADC for battery (err %d)", ret);
        return ret;
    }

    // 12-bit SAADC (0 to 4095 range).
    // Internal reference = 600mV, gain = 1/6 -> Full-scale input voltage = 3600mV.
    // The battery voltage divider resistor values are 1M and 1M, which divides by 2.
    // So the actual battery voltage is 2 * measured voltage.
    // Actual Full-scale = 3600mV * 2 = 7200mV.
    int32_t val_mv = ((int32_t)raw_val * 7200) / 4096;
    *voltage_mv = (uint32_t)val_mv;

    return 0;
}

bool battery_drv_is_charging(void)
{
    if (gpio0_dev == NULL) {
        return false;
    }
    // Pin is active-low (0 = charging, 1 = not charging/disconnected)
    return gpio_pin_get(gpio0_dev, CHARGE_STAT_PIN) == 0;
}
