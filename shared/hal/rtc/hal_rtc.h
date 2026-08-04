#ifndef HAL_RTC_H
#define HAL_RTC_H

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>

/**
 * @brief Initialize the RTC counter device.
 * 
 * @param counter_dev Pointer to the counter device structure.
 * @return int 0 on success, negative errno on failure.
 */
int hal_rtc_init(const struct device *counter_dev);

/**
 * @brief Set the counter time (ticks/seconds).
 * 
 * @param counter_dev Pointer to the counter device structure.
 * @param time_sec Time value in seconds.
 * @return int 0 on success, negative errno on failure.
 */
int hal_rtc_set_time(const struct device *counter_dev, uint32_t time_sec);

/**
 * @brief Get the current time in seconds.
 * 
 * @param counter_dev Pointer to the counter device structure.
 * @param time_sec Pointer to store the time value in seconds.
 * @return int 0 on success, negative errno on failure.
 */
int hal_rtc_get_time(const struct device *counter_dev, uint32_t *time_sec);

/**
 * @brief Configure an alarm interrupt.
 * 
 * @param counter_dev Pointer to the counter device structure.
 * @param delay_sec Delay in seconds from current time.
 * @param callback Callback function to run when alarm fires.
 * @param user_data User data pointer.
 * @return int 0 on success, negative errno on failure.
 */
int hal_rtc_set_alarm(const struct device *counter_dev, uint32_t delay_sec,
                      counter_alarm_callback_t callback, void *user_data);

#endif // HAL_RTC_H
