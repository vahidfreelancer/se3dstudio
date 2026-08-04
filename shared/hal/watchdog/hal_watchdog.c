#include "hal_watchdog.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hal_wdt, LOG_LEVEL_INF);

int hal_wdt_init(const struct device *wdt_dev)
{
    if (wdt_dev == NULL) {
        LOG_ERR("Watchdog device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(wdt_dev)) {
        LOG_ERR("Watchdog device %s is not ready", wdt_dev->name);
        return -ENODEV;
    }
    return 0;
}

int hal_wdt_install_timeout(const struct device *wdt_dev, uint32_t timeout_ms)
{
    if (wdt_dev == NULL) {
        return -EINVAL;
    }

    struct wdt_timeout_cfg wdt_config = {
        .flags = WDT_FLAG_RESET_SOC,
        .window.min = 0,
        .window.max = timeout_ms,
    };

    int channel_id = wdt_install_timeout(wdt_dev, &wdt_config);
    if (channel_id < 0) {
        LOG_ERR("Failed to install watchdog timeout (err %d)", channel_id);
    }
    return channel_id;
}

int hal_wdt_feed(const struct device *wdt_dev, int channel_id)
{
    if (wdt_dev == NULL) {
        return -EINVAL;
    }
    return wdt_feed(wdt_dev, channel_id);
}

int hal_wdt_start(const struct device *wdt_dev)
{
    if (wdt_dev == NULL) {
        return -EINVAL;
    }
    int ret = wdt_setup(wdt_dev, WDT_OPT_PAUSE_IN_SLEEP);
    if (ret < 0) {
        LOG_ERR("Failed to start watchdog (err %d)", ret);
    }
    return ret;
}
