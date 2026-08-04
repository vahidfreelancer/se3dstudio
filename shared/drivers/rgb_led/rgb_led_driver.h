#ifndef RGB_LED_DRIVER_H
#define RGB_LED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the onboard RGB LEDs (Red, Green, Blue).
 * 
 * @return int 0 on success, negative errno on failure.
 */
int rgb_led_init(void);

/**
 * @brief Set the state of each color channel.
 * 
 * @param r Red channel state (true = ON, false = OFF).
 * @param g Green channel state (true = ON, false = OFF).
 * @param b Blue channel state (true = ON, false = OFF).
 * @return int 0 on success, negative errno on failure.
 */
int rgb_led_set(bool r, bool g, bool b);

/**
 * @brief Toggle the state of each color channel.
 * 
 * @param r Red channel toggle.
 * @param g Green channel toggle.
 * @param b Blue channel toggle.
 * @return int 0 on success, negative errno on failure.
 */
int rgb_led_toggle(bool r, bool g, bool b);

/**
 * @brief Set RGB state using a 3-bit color mask.
 * Mask format: bit 0: Red, bit 1: Green, bit 2: Blue.
 * 
 * @param color_mask The 3-bit color mask.
 * @return int 0 on success, negative errno on failure.
 */
int rgb_led_set_mask(uint8_t color_mask);

#endif // RGB_LED_DRIVER_H
