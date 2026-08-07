#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_gpio.h>
#include "imu_driver.h"
#include "ble_driver.h"
#include "rgb_led_driver.h"
#include "mic_driver.h"
#include "ekf.h"

LOG_MODULE_REGISTER(app_ble_sensor, LOG_LEVEL_INF);

#define SAMPLE_PERIOD_MS 33 // ~30Hz IMU loop
#define PCM_BLOCK_SAMPLES 256 // 256 samples of 16-bit PCM (512 bytes)
#define ADPCM_BLOCK_BYTES 128 // 256 samples compressed to 128 4-bit ADPCM bytes

static const int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3326, 3659, 4025, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

struct adpcm_state {
    int16_t valprev;
    int8_t index;
};

static uint8_t adpcm_encode_sample(int16_t sample, struct adpcm_state *state)
{
    int diff = sample - state->valprev;
    int step = stepsizeTable[state->index];
    int code = 0;
    int tempstep = step;

    if (diff < 0) {
        code = 8;
        diff = -diff;
    }

    if (diff >= tempstep) {
        code |= 4;
        diff -= tempstep;
    }
    tempstep >>= 1;
    if (diff >= tempstep) {
        code |= 2;
        diff -= tempstep;
    }
    tempstep >>= 1;
    if (diff >= tempstep) {
        code |= 1;
    }

    int vpdiff = step >> 3;
    if (code & 4) vpdiff += step;
    if (code & 2) vpdiff += step >> 1;
    if (code & 1) vpdiff += step >> 2;

    if (code & 8) {
        state->valprev -= vpdiff;
    } else {
        state->valprev += vpdiff;
    }

    if (state->valprev > 32767) state->valprev = 32767;
    else if (state->valprev < -32768) state->valprev = -32768;

    state->index += indexTable[code & 7];
    if (state->index < 0) state->index = 0;
    else if (state->index > 88) state->index = 88;

    return code;
}

static void adpcm_encode_block(const int16_t *pcm, size_t len, uint8_t *encoded, struct adpcm_state *state)
{
    for (size_t i = 0; i < len; i += 2) {
        uint8_t code1 = adpcm_encode_sample(pcm[i], state);
        uint8_t code2 = adpcm_encode_sample(pcm[i + 1], state);
        encoded[i / 2] = (code2 << 4) | (code1 & 0x0F);
    }
}

K_THREAD_STACK_DEFINE(mic_stack_area, 2048);
static struct k_thread mic_thread_data;
static volatile bool mic_active = false;

static int16_t pcm_buf[PCM_BLOCK_SAMPLES];
static uint8_t adpcm_buf[ADPCM_BLOCK_BYTES];
static struct adpcm_state compress_state;

/* Early power-on hardware hook at PRE_KERNEL_1 */
static int early_power_on_hardware(void)
{
    // On XIAO BLE Sense, P1.08 LOW (0) powers ON the LSM6DS3TR-C IMU regulator (P-FET)
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(1, 8));
    nrf_gpio_pin_write(NRF_GPIO_PIN_MAP(1, 8), 0);
    
    // On XIAO BLE Sense, P1.10 HIGH (1) powers ON the MSM261D3526H PDM microphone regulator
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(1, 10));
    nrf_gpio_pin_write(NRF_GPIO_PIN_MAP(1, 10), 1);

    return 0;
}

SYS_INIT(early_power_on_hardware, PRE_KERNEL_1, 0);

static inline float sensor_val_to_float(const struct sensor_value *val)
{
    return (float)val->val1 + (float)val->val2 / 1000000.0f;
}

static void on_ble_write(const uint8_t *data, uint16_t len)
{
    if (len > 0 && data[0] == 0x01) {
        LOG_INF("Received Gyro Calibration Command from BLE Client");
        ekf_start_calibration();
    }
}

/* Dedicated Microphone Thread with BLE TX Buffer Flow Control */
static void mic_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("ADPCM Audio Microphone worker thread started");

    while (1) {
        if (ble_drv_is_connected() && ble_drv_is_audio_notify_enabled()) {
            if (!mic_active) {
                memset(&compress_state, 0, sizeof(compress_state));
                int ret = mic_drv_start();
                if (ret == 0) {
                    mic_active = true;
                    LOG_INF("Microphone PDM ADPCM stream started");
                } else {
                    LOG_ERR("Failed to start mic driver: %d", ret);
                    k_msleep(200);
                    continue;
                }
            }

            // Read 256 PCM samples (512 bytes, blocks for ~16ms while buffer fills)
            int ret = mic_drv_read(pcm_buf, PCM_BLOCK_SAMPLES);
            if (ret == 0) {
                // Compress 256 16-bit PCM samples to 128 4-bit ADPCM bytes
                adpcm_encode_block(pcm_buf, PCM_BLOCK_SAMPLES, adpcm_buf, &compress_state);

                // Stream 128 bytes ADPCM packet over BLE
                int notify_ret = ble_drv_notify_audio(adpcm_buf, sizeof(adpcm_buf));
                if (notify_ret < 0) {
                    // If BLE TX buffers full, yield briefly for radio transmission
                    k_msleep(15);
                }
            } else {
                k_msleep(10);
            }
        } else {
            if (mic_active) {
                mic_drv_stop();
                mic_active = false;
                LOG_INF("Microphone stream stopped");
            }
            k_msleep(100);
        }
    }
}

int main(void)
{
    int ret;

    // Delay for hardware power rail stabilization
    k_msleep(100);

    LOG_INF("Starting Seeed Studio XIAO nRF52840 BLE IMU + ADPCM Audio Application...");

    // Initialize RGB LED driver
    ret = rgb_led_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize RGB LED driver: %d", ret);
    }
    rgb_led_set(true, false, false);

    // Initialize IMU driver
    ret = imu_driver_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize IMU driver: %d", ret);
    }

    // Initialize Extended Kalman Filter
    ekf_init();

    // Initialize PDM Microphone driver
    ret = mic_drv_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize microphone driver: %d", ret);
    }

    // Start dedicated Preemptible ADPCM Microphone Thread
    k_thread_create(&mic_thread_data, mic_stack_area,
                    K_THREAD_STACK_SIZEOF(mic_stack_area),
                    mic_thread_entry, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

    // Initialize BLE driver
    ret = ble_drv_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize BLE driver: %d", ret);
    }

    // Set BLE write callback for calibration commands
    ble_drv_set_write_cb(on_ble_write);

    // Start BLE Advertising as se3dstudio_imu_01
    ret = ble_drv_adv_start("se3dstudio_imu_01");
    if (ret < 0) {
        LOG_ERR("Failed to start BLE advertising: %d", ret);
    }

    struct imu_data raw_imu;
    imu_ble_payload_t payload;
    uint32_t cal_blink_counter = 0;
    bool cal_green_flash = false;

    // Main 30Hz IMU loop
    while (1) {
        ret = imu_driver_fetch(&raw_imu);
        if (ret == 0) {
            float ax = sensor_val_to_float(&raw_imu.accel[0]);
            float ay = sensor_val_to_float(&raw_imu.accel[1]);
            float az = sensor_val_to_float(&raw_imu.accel[2]);
            float gx = sensor_val_to_float(&raw_imu.gyro[0]);
            float gy = sensor_val_to_float(&raw_imu.gyro[1]);
            float gz = sensor_val_to_float(&raw_imu.gyro[2]);

            if (ekf_is_calibrating()) {
                bool cal_done = ekf_update_calibration(gx, gy, gz);

                cal_blink_counter++;
                if ((cal_blink_counter / 5) % 2 == 0) {
                    rgb_led_set(true, true, false); // Yellow
                } else {
                    rgb_led_set(false, false, false); // OFF
                }

                if (cal_done) {
                    LOG_INF("Gyro Calibration Complete!");
                    rgb_led_set(false, true, false);
                    cal_green_flash = true;
                    k_msleep(500);
                }
            } else {
                ekf_update(ax, ay, az, gx, gy, gz, (float)SAMPLE_PERIOD_MS / 1000.0f);
                ekf_get_angles(&payload.roll, &payload.pitch, &payload.yaw);

                payload.accel_x = ax;
                payload.accel_y = ay;
                payload.accel_z = az;
                payload.gyro_x = gx;
                payload.gyro_y = gy;
                payload.gyro_z = gz;

                if (ble_drv_is_connected()) {
                    ble_drv_notify((const uint8_t *)&payload, sizeof(payload));
                    if (!cal_green_flash) {
                        rgb_led_set(false, false, true); // Blue = Connected
                    }
                } else {
                    if (!cal_green_flash) {
                        rgb_led_set(true, false, false); // Red = Disconnected
                    }
                }
                cal_green_flash = false;
            }
        } else {
            imu_driver_init();
            if (ble_drv_is_connected()) {
                rgb_led_set(false, false, true);
            } else {
                rgb_led_set(true, false, false);
            }
        }

        k_msleep(SAMPLE_PERIOD_MS);
    }

    return 0;
}
