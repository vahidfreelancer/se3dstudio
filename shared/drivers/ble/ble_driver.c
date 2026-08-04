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

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);

static uint8_t char_value[20] = {0};

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
    return len;
}

/* Custom GATT Service Definition */
BT_GATT_SERVICE_DEFINE(custom_srv,
    BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
    BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_custom_char, write_custom_char, char_value),
);

static struct bt_conn *default_conn = NULL;

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("BLE Connection failed (err 0x%02x)", err);
    } else {
        default_conn = bt_conn_ref(conn);
        LOG_INF("BLE Connected");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("BLE Disconnected (reason 0x%02x)", reason);
    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int ble_drv_init(void)
{
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
    struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_NAME_COMPLETE, device_name, strlen(device_name)),
    };

    struct bt_data sd[] = {
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
    };

    int ret = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (ret) {
        LOG_ERR("Advertising failed to start (err %d)", ret);
        return ret;
    }

    LOG_INF("BLE Advertising started as '%s'", device_name);
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
    if (len > sizeof(char_value)) {
        return -EINVAL;
    }
    memcpy(char_value, value, len);

    // Notify attribute at index 2 (value attribute)
    int ret = bt_gatt_notify(NULL, &custom_srv.attrs[2], char_value, len);
    if (ret < 0 && ret != -ENOTCONN) {
        LOG_ERR("Failed to send BLE notification (err %d)", ret);
        return ret;
    }
    return 0;
}
