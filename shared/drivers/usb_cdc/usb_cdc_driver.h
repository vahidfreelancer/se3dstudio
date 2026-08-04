#ifndef USB_CDC_DRIVER_H
#define USB_CDC_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/drivers/uart.h>

/**
 * @brief Initialize the USB CDC driver and enable the USB stack.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int usb_cdc_drv_init(void);

/**
 * @brief Transmit data over USB CDC ACM (polling/blocking).
 * 
 * @param data Data buffer to transmit.
 * @param len Length of data to transmit.
 * @return int 0 on success, negative errno on failure.
 */
int usb_cdc_drv_tx(const uint8_t *data, size_t len);

/**
 * @brief Set receive callback for USB CDC ACM device.
 * 
 * @param cb Callback function.
 * @param user_data Pointer to pass to the callback.
 * @return int 0 on success, negative errno on failure.
 */
int usb_cdc_drv_set_rx_callback(uart_irq_callback_user_data_t cb, void *user_data);

#endif // USB_CDC_DRIVER_H
