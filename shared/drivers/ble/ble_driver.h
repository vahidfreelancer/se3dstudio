#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Initialize the Bluetooth Low Energy stack.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int ble_drv_init(void);

/**
 * @brief Start BLE advertising.
 * 
 * @param device_name The advertising name to broadcast.
 * @return int 0 on success, negative errno on failure.
 */
int ble_drv_adv_start(const char *device_name);

/**
 * @brief Stop BLE advertising.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int ble_drv_adv_stop(void);

typedef void (*ble_write_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief Register a callback for incoming GATT Characteristic Write requests.
 * 
 * @param cb Callback function pointer.
 */
void ble_drv_set_write_cb(ble_write_cb_t cb);

/**
 * @brief Check if a BLE central client is currently connected.
 * 
 * @return true if connected, false otherwise.
 */
bool ble_drv_is_connected(void);

/**
 * @brief Check if BLE audio notifications (CCCD) are enabled by client.
 * 
 * @return true if notifications enabled, false otherwise.
 */
bool ble_drv_is_audio_notify_enabled(void);

/**
 * @brief Send a GATT notification to connected clients.
 * 
 * @param value The value to notify.
 * @param len Length of the value.
 * @return int 0 on success, negative errno on failure.
 */
int ble_drv_notify(const uint8_t *value, uint16_t len);

/**
 * @brief Send an Audio GATT notification to connected clients.
 * 
 * @param value Audio buffer payload.
 * @param len Length of audio buffer.
 * @return int 0 on success, negative errno on failure.
 */
int ble_drv_notify_audio(const uint8_t *value, uint16_t len);

#endif // BLE_DRIVER_H
