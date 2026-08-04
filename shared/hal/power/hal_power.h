#ifndef HAL_POWER_H
#define HAL_POWER_H

#include <zephyr/device.h>

/**
 * @brief Enter low power mode.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int hal_power_enter_low_power(void);

/**
 * @brief Shut down the system (Deep sleep / System Off).
 * This will put the nRF52840 in SYSTEM OFF mode, where it can only wake up via reset or GPIO event.
 */
void hal_power_system_off(void);

/**
 * @brief Set power state of a specific peripheral device.
 * 
 * @param dev Pointer to the device.
 * @param action PM action / state (e.g. PM_DEVICE_ACTION_SUSPEND).
 * @return int 0 on success, negative errno on failure.
 */
int hal_power_set_device_state(const struct device *dev, uint32_t action);

#endif // HAL_POWER_H
