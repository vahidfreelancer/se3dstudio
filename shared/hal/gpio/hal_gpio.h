#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <zephyr/drivers/gpio.h>

/**
 * @brief Initialize and configure a GPIO pin spec.
 * 
 * @param spec Pointer to the GPIO specification structure.
 * @param extra_flags Flags to apply (e.g. GPIO_INPUT, GPIO_OUTPUT).
 * @return int 0 on success, negative errno on failure.
 */
int hal_gpio_configure(const struct gpio_dt_spec *spec, gpio_flags_t extra_flags);

/**
 * @brief Set the output state of a GPIO pin.
 * 
 * @param spec Pointer to the GPIO specification structure.
 * @param value The value to set (0 or 1).
 * @return int 0 on success, negative errno on failure.
 */
int hal_gpio_write(const struct gpio_dt_spec *spec, int value);

/**
 * @brief Read the current state of a GPIO pin.
 * 
 * @param spec Pointer to the GPIO specification structure.
 * @return int The pin state (0 or 1) on success, negative errno on failure.
 */
int hal_gpio_read(const struct gpio_dt_spec *spec);

/**
 * @brief Toggle the state of a GPIO pin.
 * 
 * @param spec Pointer to the GPIO specification structure.
 * @return int 0 on success, negative errno on failure.
 */
int hal_gpio_toggle(const struct gpio_dt_spec *spec);

/**
 * @brief Configure an interrupt on a GPIO pin.
 * 
 * @param spec Pointer to the GPIO specification.
 * @param flags Interrupt configuration flags (e.g., GPIO_INT_EDGE_TO_ACTIVE).
 * @param handler Callback function to invoke on interrupt.
 * @param cb Pointer to the gpio_callback structure (owned by caller, must persist).
 * @return int 0 on success, negative errno on failure.
 */
int hal_gpio_configure_interrupt(const struct gpio_dt_spec *spec,
                                 gpio_flags_t flags,
                                 gpio_callback_handler_t handler,
                                 struct gpio_callback *cb);

#endif // HAL_GPIO_H
