#include "hal_rtc.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hal_rtc, LOG_LEVEL_INF);

static uint32_t rtc_time_offset_sec = 0;

int hal_rtc_init(const struct device *counter_dev)
{
    if (counter_dev == NULL) {
        LOG_ERR("RTC counter device is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(counter_dev)) {
        LOG_ERR("RTC counter device %s is not ready", counter_dev->name);
        return -ENODEV;
    }
    return 0;
}

int hal_rtc_set_time(const struct device *counter_dev, uint32_t time_sec)
{
    if (counter_dev == NULL) {
        return -EINVAL;
    }
    uint32_t freq = counter_get_frequency(counter_dev);
    if (freq == 0) {
        return -ENOTSUP;
    }
    
    uint32_t ticks = 0;
    int ret = counter_get_value(counter_dev, &ticks);
    if (ret < 0) {
        return ret;
    }
    
    uint32_t current_sec = ticks / freq;
    rtc_time_offset_sec = time_sec - current_sec;
    return 0;
}

int hal_rtc_get_time(const struct device *counter_dev, uint32_t *time_sec)
{
    if (counter_dev == NULL || time_sec == NULL) {
        return -EINVAL;
    }
    
    uint32_t freq = counter_get_frequency(counter_dev);
    if (freq == 0) {
        return -ENOTSUP;
    }
    
    uint32_t ticks = 0;
    int ret = counter_get_value(counter_dev, &ticks);
    if (ret < 0) {
        return ret;
    }
    
    *time_sec = rtc_time_offset_sec + (ticks / freq);
    return 0;
}

int hal_rtc_set_alarm(const struct device *counter_dev, uint32_t delay_sec,
                      counter_alarm_callback_t callback, void *user_data)
{
    if (counter_dev == NULL) {
        return -EINVAL;
    }
    
    uint32_t freq = counter_get_frequency(counter_dev);
    if (freq == 0) {
        return -ENOTSUP;
    }
    
    struct counter_alarm_cfg alarm_cfg = {
        .flags = 0, // Relative alarm
        .ticks = delay_sec * freq,
        .callback = callback,
        .user_data = user_data,
    };
    
    int ret = counter_set_channel_alarm(counter_dev, 0, &alarm_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to set RTC alarm (err %d)", ret);
    }
    return ret;
}
