#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include <stdint.h>
#include <stddef.h>

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

/**
 * @brief Send a GATT notification to connected clients.
 * 
 * @param value The value to notify.
 * @param len Length of the value.
 * @return int 0 on success, negative errno on failure.
 */
int ble_drv_notify(const uint8_t *value, uint16_t len);

#endif // BLE_DRIVER_H
