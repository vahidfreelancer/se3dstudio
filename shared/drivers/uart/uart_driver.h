#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the UART driver and set a receive callback.
 * 
 * @param dev Pointer to the UART device structure.
 * @param cb Receive callback function.
 * @param user_data Pointer to pass to the callback function.
 * @return int 0 on success, negative errno on failure.
 */
int uart_drv_init(const struct device *dev, uart_irq_callback_user_data_t cb, void *user_data);

/**
 * @brief Transmit data over UART (polling/blocking).
 * 
 * @param dev Pointer to the UART device structure.
 * @param data Data buffer to transmit.
 * @param len Length of the data buffer.
 * @return int 0 on success, negative errno on failure.
 */
int uart_drv_tx(const struct device *dev, const uint8_t *data, size_t len);

/**
 * @brief Enable interrupt-driven receive on UART.
 * 
 * @param dev Pointer to the UART device structure.
 * @return int 0 on success, negative errno on failure.
 */
int uart_drv_rx_enable(const struct device *dev);

/**
 * @brief Disable interrupt-driven receive on UART.
 * 
 * @param dev Pointer to the UART device structure.
 * @return int 0 on success, negative errno on failure.
 */
int uart_drv_rx_disable(const struct device *dev);

#endif // UART_DRIVER_H
