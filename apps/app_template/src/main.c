#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>

/* Include shared libraries */
#include "imu_driver.h"
#include "utils.h"

/* Include new core HAL libraries */
#include "hal_gpio.h"
#include "hal_adc.h"
#include "hal_pwm.h"
#include "hal_timer.h"
#include "hal_rtc.h"
#include "hal_watchdog.h"
#include "hal_flash.h"
#include "hal_rng.h"
#include "hal_system.h"
#include "hal_power.h"

#if defined(CONFIG_ADC_NRFX_SAADC)
#include <hal/nrf_saadc.h>
#endif

/* Register application log module */
LOG_MODULE_REGISTER(app_template, LOG_LEVEL_INF);

/* Get the LED0 devicetree node alias (typically Red LED on Seeed Studio XIAO boards) */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_EXISTS(LED0_NODE)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif

/* Define a kernel timer for our timer HAL test */
static struct k_timer test_timer;
static void test_timer_expiry_fn(struct k_timer *timer_id)
{
    LOG_INF("HAL Timer callback fired! (Every 5 seconds)");
}

int main(void)
{
    int ret;

    LOG_INF("Starting Application Template on Seeed Studio XIAO nRF52840 Sense Plus...");

    /* Log System Hardware Details */
    hal_system_log_info();

    /* Initialize user LED if configured in the devicetree using HAL GPIO */
#if DT_NODE_EXISTS(LED0_NODE)
    ret = hal_gpio_configure(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED pin via HAL GPIO (err %d)", ret);
    } else {
        LOG_INF("LED configured successfully via HAL GPIO");
    }
#endif

    /* Initialize shared IMU driver wrapper */
    ret = imu_driver_init();
    if (ret < 0) {
        LOG_WRN("Failed to initialize IMU driver (err %d). Running in degraded mode.", ret);
    } else {
        LOG_INF("IMU driver initialized successfully");
    }

    /* Initialize and start Watchdog */
    const struct device *const wdt_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(wdt0));
    int wdt_channel = -1;
    if (wdt_dev != NULL && device_is_ready(wdt_dev)) {
        ret = hal_wdt_init(wdt_dev);
        if (ret == 0) {
            wdt_channel = hal_wdt_install_timeout(wdt_dev, 5000); // 5-second timeout
            if (wdt_channel >= 0) {
                ret = hal_wdt_start(wdt_dev);
                if (ret == 0) {
                    LOG_INF("Watchdog started successfully with 5000ms timeout");
                }
            }
        }
    } else {
        LOG_WRN("Watchdog device not ready or disabled");
    }

    /* Initialize HAL Timer (k_timer wrapper) */
    hal_timer_init(&test_timer, test_timer_expiry_fn, NULL);
    hal_timer_start(&test_timer, 5000, 5000); // Start 5s periodic timer

    /* Initialize RTC using rtc1 counter */
    const struct device *const rtc_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(rtc1));
    bool rtc_available = false;
    if (rtc_dev != NULL && device_is_ready(rtc_dev)) {
        ret = hal_rtc_init(rtc_dev);
        if (ret == 0) {
            rtc_available = true;
            // Set RTC time to 17171717 seconds (arbitrary time)
            hal_rtc_set_time(rtc_dev, 17171717);
            LOG_INF("RTC initialized and time set to 17171717 seconds");
        }
    } else {
        LOG_WRN("RTC counter device not ready or disabled");
    }

    /* Initialize ADC */
    const struct device *const adc_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(adc));
    bool adc_available = false;
    if (adc_dev != NULL && device_is_ready(adc_dev)) {
        ret = hal_adc_init(adc_dev);
        if (ret == 0) {
            adc_available = true;
            LOG_INF("ADC driver initialized successfully");
        }
    } else {
        LOG_WRN("ADC device not ready or disabled");
    }

    struct imu_data imu_readings;

    while (1) {
        /* Toggle user LED using HAL GPIO */
#if DT_NODE_EXISTS(LED0_NODE)
        hal_gpio_toggle(&led);
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
            }
        }

        /* Test RNG: Print random 32-bit integer */
        uint32_t rand_val = hal_rng_get_u32();
        LOG_INF("HAL RNG Value: 0x%08X", rand_val);

        /* Test RTC: Get current time */
        if (rtc_available) {
            uint32_t rtc_time = 0;
            if (hal_rtc_get_time(rtc_dev, &rtc_time) == 0) {
                LOG_INF("HAL RTC Time: %u seconds", rtc_time);
            }
        }

        /* Test ADC: Read a channel if available (channel 0) */
        if (adc_available) {
            int16_t adc_raw = 0;
            int32_t adc_mv = 0;
            // Read channel 0 with 10-bit resolution
            // Configure the channel first
            struct adc_channel_cfg ch_cfg = {
                .gain = ADC_GAIN_1_6,
                .reference = ADC_REF_INTERNAL, // 0.6V internal ref on nRF52
                .acquisition_time = ADC_ACQ_TIME_DEFAULT,
                .channel_id = 0,
#if defined(CONFIG_ADC_NRFX_SAADC)
                .input_positive = NRF_SAADC_INPUT_AIN0, // AIN0
#endif
            };
            hal_adc_channel_setup(adc_dev, &ch_cfg);
            
            if (hal_adc_read_raw(adc_dev, 0, 10, &adc_raw) == 0) {
                // nRF52 internal reference is 600mV, with gain 1/6, the full-scale range is 3600mV
                hal_adc_read_mv(adc_dev, 0, 10, 3600, &adc_mv);
                LOG_INF("HAL ADC Channel 0 Raw: %d, Voltage: %d mV", adc_raw, adc_mv);
            }
        }

        /* Feed Watchdog if active */
        if (wdt_channel >= 0) {
            hal_wdt_feed(wdt_dev, wdt_channel);
        }

        k_msleep(1000);
    }

    return 0;
}
