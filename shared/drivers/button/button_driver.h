#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include <zephyr/drivers/gpio.h>

/**
 * @brief Initialize a button GPIO pin and set a press callback.
 * 
 * @param spec Pointer to the GPIO specification for the button.
 * @param handler Callback function to invoke on button press.
 * @param cb Pointer to the gpio_callback structure (owned by caller, must persist).
 * @return int 0 on success, negative errno on failure.
 */
int button_drv_init(const struct gpio_dt_spec *spec,
                    gpio_callback_handler_t handler,
                    struct gpio_callback *cb);

/**
 * @brief Read the current physical state of the button.
 * 
 * @param spec Pointer to the GPIO specification for the button.
 * @return int 1 if pressed, 0 if not pressed, or negative errno on failure.
 */
int button_drv_read(const struct gpio_dt_spec *spec);

#endif // BUTTON_DRIVER_H
