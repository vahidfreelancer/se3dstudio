#include "hal_power.h"
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_SOC_SERIES_NRF52X)
#include <hal/nrf_power.h>
#endif

LOG_MODULE_REGISTER(hal_power, LOG_LEVEL_INF);

int hal_power_enter_low_power(void)
{
    k_cpu_idle();
    return 0;
}

void hal_power_system_off(void)
{
    LOG_INF("Entering System Off (Deep Sleep)...");
    k_sleep(K_MSEC(100)); // Allow logs to flush

#if defined(CONFIG_PM)
    // Force SOFT_OFF state
    pm_state_force(0, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
    k_sleep(K_FOREVER);
#elif defined(CONFIG_SOC_SERIES_NRF52X)
    nrf_power_system_off(NRF_POWER);
    while (1) {
        // Wait for system off
    }
#else
    LOG_WRN("System Off not supported on this SoC/Configuration");
#endif
}

int hal_power_set_device_state(const struct device *dev, uint32_t action)
{
    if (dev == NULL) {
        return -EINVAL;
    }
#ifdef CONFIG_PM_DEVICE
    int ret = pm_device_action_run(dev, action);
    if (ret < 0) {
        LOG_ERR("Failed to set device %s power state (err %d)", dev->name, ret);
    }
    return ret;
#else
    LOG_WRN("Device Power Management (CONFIG_PM_DEVICE) is disabled");
    return -ENOTSUP;
#endif
}
