#ifndef BATTERY_DRIVER_H
#define BATTERY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the battery measurement GPIOs and ADC.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int battery_drv_init(void);

/**
 * @brief Enable the battery voltage divider.
 * 
 * @param enable true to enable divider (draws current), false to disable (save power).
 * @return int 0 on success, negative errno on failure.
 */
int battery_drv_divider_enable(bool enable);

/**
 * @brief Measure the battery voltage in millivolts.
 * 
 * @param voltage_mv Pointer to store the measured voltage in millivolts.
 * @return int 0 on success, negative errno on failure.
 */
int battery_drv_read_voltage(uint32_t *voltage_mv);

/**
 * @brief Check if the battery is currently charging.
 * 
 * @return bool true if charging, false if not charging or disconnected.
 */
bool battery_drv_is_charging(void);

#endif // BATTERY_DRIVER_H
