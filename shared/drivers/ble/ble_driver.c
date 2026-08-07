#include "ble_driver.h"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_driver, LOG_LEVEL_INF);

#define BT_UUID_CUSTOM_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_CUSTOM_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

#define BT_UUID_AUDIO_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);
static struct bt_uuid_128 audio_char_uuid = BT_UUID_INIT_128(BT_UUID_AUDIO_CHAR_VAL);

static uint8_t char_value[256] = {0};
static uint8_t audio_char_value[240] = {0};
static struct bt_conn *default_conn = NULL;
static ble_write_cb_t write_cb = NULL;
static char current_dev_name[32] = "se3dstudio_imu_01";

static volatile bool audio_notifications_enabled = false;
static struct k_work adv_work;

static void adv_work_handler(struct k_work *work)
{
    LOG_INF("Restarting BLE advertising...");
    bt_le_adv_stop();
    ble_drv_adv_start(current_dev_name);
}

void ble_drv_set_write_cb(ble_write_cb_t cb)
{
    write_cb = cb;
}

bool ble_drv_is_connected(void)
{
    return (default_conn != NULL);
}

bool ble_drv_is_audio_notify_enabled(void)
{
    return audio_notifications_enabled;
}

static ssize_t read_custom_char(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, char_value, sizeof(char_value));
}

static ssize_t write_custom_char(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len, uint16_t offset,
                                  uint8_t flags)
{
    if (offset + len > sizeof(char_value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    memcpy(char_value + offset, buf, len);
    LOG_INF("BLE Characteristic Written: %d bytes", len);
    if (write_cb) {
        write_cb((const uint8_t *)buf, len);
    }
    return len;
}

static void imu_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    LOG_INF("IMU Notification CCCD updated: 0x%04x", value);
}

static void audio_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    audio_notifications_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Audio Notification CCCD State: %s", audio_notifications_enabled ? "ENABLED" : "DISABLED");
}

/* Custom GATT Service Definition */
BT_GATT_SERVICE_DEFINE(custom_srv,
    BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
    BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_custom_char, write_custom_char, char_value),
    BT_GATT_CCC(imu_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&audio_char_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           NULL, NULL, audio_char_value),
    BT_GATT_CCC(audio_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("BLE Connection failed (err 0x%02x)", err);
    } else {
        if (default_conn) {
            bt_conn_unref(default_conn);
        }
        default_conn = bt_conn_ref(conn);
        LOG_INF("BLE Connected");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("BLE Disconnected (reason 0x%02x)", reason);
    audio_notifications_enabled = false;
    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
    // Schedule advertising restart cleanly via system workqueue
    k_work_submit(&adv_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int ble_drv_init(void)
{
    k_work_init(&adv_work, adv_work_handler);
    int ret = bt_enable(NULL);
    if (ret && ret != -EALREADY) {
        LOG_ERR("Bluetooth init failed (err %d)", ret);
        return ret;
    }
    LOG_INF("Bluetooth initialized successfully");
    return 0;
}

int ble_drv_adv_start(const char *device_name)
{
    if (device_name) {
        strncpy(current_dev_name, device_name, sizeof(current_dev_name) - 1);
        current_dev_name[sizeof(current_dev_name) - 1] = '\0';
    }
    struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_NAME_COMPLETE, current_dev_name, strlen(current_dev_name)),
    };

    struct bt_data sd[] = {
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
    };

    // Ensure stale advertising is stopped first
    bt_le_adv_stop();

    int ret = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (ret && ret != -EALREADY) {
        LOG_ERR("Advertising failed to start (err %d)", ret);
        return ret;
    }

    LOG_INF("BLE Advertising started as '%s'", current_dev_name);
    return 0;
}

int ble_drv_adv_stop(void)
{
    int ret = bt_le_adv_stop();
    if (ret) {
        LOG_ERR("Failed to stop advertising (err %d)", ret);
        return ret;
    }
    LOG_INF("BLE Advertising stopped");
    return 0;
}

int ble_drv_notify(const uint8_t *value, uint16_t len)
{
    if (!default_conn) {
        return -ENOTCONN;
    }
    if (len > sizeof(char_value)) {
        return -EINVAL;
    }
    memcpy(char_value, value, len);

    // Notify attribute at index 2 (IMU Characteristic Value Attribute)
    return bt_gatt_notify(default_conn, &custom_srv.attrs[2], char_value, len);
}

int ble_drv_notify_audio(const uint8_t *value, uint16_t len)
{
    if (!default_conn) {
        return -ENOTCONN;
    }
    if (len > sizeof(audio_char_value)) {
        return -EINVAL;
    }
    memcpy(audio_char_value, value, len);

    // Notify attribute at index 5 (Audio Characteristic Value Attribute)
    return bt_gatt_notify(default_conn, &custom_srv.attrs[5], audio_char_value, len);
}
