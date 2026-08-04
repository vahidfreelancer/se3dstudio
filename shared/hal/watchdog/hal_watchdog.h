#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>

/**
 * @brief Initialize the watchdog device.
 * 
 * @param wdt_dev Pointer to the watchdog device structure.
 * @return int 0 on success, negative errno on failure.
 */
int hal_wdt_init(const struct device *wdt_dev);

/**
 * @brief Install and configure a watchdog timeout channel.
 * 
 * @param wdt_dev Pointer to the watchdog device structure.
 * @param timeout_ms Timeout period in milliseconds.
 * @return int Channel ID (>0) on success, negative errno on failure.
 */
int hal_wdt_install_timeout(const struct device *wdt_dev, uint32_t timeout_ms);

/**
 * @brief Feed a watchdog channel.
 * 
 * @param wdt_dev Pointer to the watchdog device structure.
 * @param channel_id Channel ID to feed.
 * @return int 0 on success, negative errno on failure.
 */
int hal_wdt_feed(const struct device *wdt_dev, int channel_id);

/**
 * @brief Start the watchdog timer.
 * 
 * @param wdt_dev Pointer to the watchdog device structure.
 * @return int 0 on success, negative errno on failure.
 */
int hal_wdt_start(const struct device *wdt_dev);

#endif // HAL_WATCHDOG_H
