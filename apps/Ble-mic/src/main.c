#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <stdio.h>
#include <string.h>

/* Include Shared Headers */
#include "hal_system.h"
#include "rgb_led_driver.h"
#include "mic_driver.h"

LOG_MODULE_REGISTER(ble_mic, LOG_LEVEL_INF);

/* ----------------------------------------------------------------- */
/* IMA ADPCM Compression State and Tables                            */
/* ----------------------------------------------------------------- */

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

    // Update valprev state
    int vpdiff = step >> 3;
    if (code & 4) vpdiff += step;
    if (code & 2) vpdiff += step >> 1;
    if (code & 1) vpdiff += step >> 2;

    if (code & 8) {
        state->valprev -= vpdiff;
    } else {
        state->valprev += vpdiff;
    }

    // Clamp valprev to 16-bit range
    if (state->valprev > 32767) state->valprev = 32767;
    else if (state->valprev < -32768) state->valprev = -32768;

    // Update stepsize index
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

/* ----------------------------------------------------------------- */
/* BLE Custom Audio GATT Service Definition                          */
/* ----------------------------------------------------------------- */

#define BT_UUID_AUDIO_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x19b10000, 0xe8f2, 0x537e, 0x4f6c, 0xd104768a1214)

#define BT_UUID_AUDIO_CHAR_VAL \
    BT_UUID_128_ENCODE(0x19b10001, 0xe8f2, 0x537e, 0x4f6c, 0xd104768a1214)

static struct bt_uuid_128 audio_service_uuid = BT_UUID_INIT_128(BT_UUID_AUDIO_SERVICE_VAL);
static struct bt_uuid_128 audio_char_uuid = BT_UUID_INIT_128(BT_UUID_AUDIO_CHAR_VAL);

static volatile bool notifications_enabled = false;
static volatile bool ble_connected = false;

static void audio_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notifications_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Audio Notification State: %s", notifications_enabled ? "Enabled" : "Disabled");
}

/* Define Custom Audio GATT Service */
BT_GATT_SERVICE_DEFINE(audio_srv,
    BT_GATT_PRIMARY_SERVICE(&audio_service_uuid),
    BT_GATT_CHARACTERISTIC(&audio_char_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(audio_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static int audio_stream_notify(const uint8_t *data, uint16_t len)
{
    return bt_gatt_notify(NULL, &audio_srv.attrs[2], data, len);
}

/* Connection Callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("BLE Connection failed (err 0x%02x)", err);
    } else {
        ble_connected = true;
        LOG_INF("BLE Client Connected");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ble_connected = false;
    notifications_enabled = false;
    LOG_INF("BLE Client Disconnected (reason 0x%02x)", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* ----------------------------------------------------------------- */
/* Main Application Entry                                            */
/* ----------------------------------------------------------------- */

int main(void)
{
    int ret;
    char ble_name[32];
    uint8_t uuid[8];
    
    int16_t pcm_buf[256];
    uint8_t adpcm_buf[128];
    struct adpcm_state compress_state;

    LOG_INF("Initializing Ble-mic Application...");

    /* Initialize LED Driver */
    ret = rgb_led_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize RGB LED (err %d)", ret);
    }
    // Set color to Red (Advertising / standby)
    rgb_led_set(true, false, false);

    /* Initialize PDM Microphone */
    ret = mic_drv_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize Microphone (err %d)", ret);
        return ret;
    }

    /* Initialize BLE Stack */
    ret = bt_enable(NULL);
    if (ret && ret != -EALREADY) {
        LOG_ERR("Bluetooth initialization failed (err %d)", ret);
        return ret;
    }
    LOG_INF("Bluetooth successfully initialized");

    /* Format advertising name: se3dstudio_mic_[XXXX] */
    int len = hal_system_get_uuid(uuid, sizeof(uuid));
    if (len == 8) {
        uint16_t dev_id = ((uint16_t)uuid[1] << 8) | uuid[0];
        snprintf(ble_name, sizeof(ble_name), "se3dstudio_mic_%04X", dev_id);
    } else {
        snprintf(ble_name, sizeof(ble_name), "se3dstudio_mic_0000");
    }

    /* Start Bluetooth Advertising */
    struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_NAME_COMPLETE, ble_name, strlen(ble_name)),
    };

    struct bt_data sd[] = {
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_AUDIO_SERVICE_VAL),
    };

    ret = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (ret < 0) {
        LOG_ERR("BLE Advertising failed to start (err %d)", ret);
        return ret;
    }
    LOG_INF("BLE Microphone advertising started as '%s'", ble_name);

    /* Streaming State Machine Loop */
    while (1) {
        if (!ble_connected) {
            // Blink Red LED to show it is advertising/disconnected
            rgb_led_set(true, false, false);
            k_msleep(500);
            rgb_led_set(false, false, false);
            k_msleep(500);
            continue;
        }

        if (!notifications_enabled) {
            // Solid Blue LED to show connected but standby
            rgb_led_set(false, false, true);
            k_msleep(100);
            continue;
        }

        // We are connected and notifications are enabled: Start audio stream
        LOG_INF("Starting Audio Recording and Streaming...");
        rgb_led_set(false, true, false); // Solid Green for recording/streaming

        // Reset ADPCM compression state before starting the stream
        memset(&compress_state, 0, sizeof(compress_state));

        ret = mic_drv_start();
        if (ret < 0) {
            LOG_ERR("Failed to start mic driver (err %d)", ret);
            k_msleep(500);
            continue;
        }

        // Active Audio Streaming Loop
        while (notifications_enabled && ble_connected) {
            // Read 256 samples (blocks here for ~16 ms while buffer fills)
            ret = mic_drv_read(pcm_buf, 256);
            if (ret < 0) {
                LOG_ERR("Failed to read audio block (err %d)", ret);
                break;
            }

            // Compress 256 16-bit PCM samples to 128 4-bit ADPCM bytes
            adpcm_encode_block(pcm_buf, 256, adpcm_buf, &compress_state);

            // Stream compressed packet over GATT notification
            ret = audio_stream_notify(adpcm_buf, 128);
            if (ret < 0) {
                if (ret != -ENOTCONN) {
                    LOG_ERR("BLE notify failed (err %d)", ret);
                }
                break;
            }
        }

        // Stream stopped: Power down microphone and return to standby
        LOG_INF("Stopping Audio Recording and Streaming...");
        mic_drv_stop();
    }

    return 0;
}
