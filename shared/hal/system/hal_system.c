#include "hal_system.h"
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#if defined(CONFIG_SOC_SERIES_NRF52X)
#include <hal/nrf_ficr.h>
#endif

LOG_MODULE_REGISTER(hal_system, LOG_LEVEL_INF);

void hal_system_reboot(void)
{
    LOG_INF("Rebooting system...");
    k_sleep(K_MSEC(100)); // Allow logs to flush
    sys_reboot(SYS_REBOOT_COLD);
}

int hal_system_get_uuid(uint8_t *id_buf, size_t max_len)
{
    if (id_buf == NULL || max_len < 8) {
        return -EINVAL;
    }

#if defined(CONFIG_SOC_SERIES_NRF52X)
    // nRF52840 has a 64-bit device ID in FICR register
    uint64_t device_id = ((uint64_t)nrf_ficr_deviceid_get(NRF_FICR, 1) << 32) |
                          nrf_ficr_deviceid_get(NRF_FICR, 0);
    memcpy(id_buf, &device_id, 8);
    return 8;
#else
    // Fallback or general identifier
    memset(id_buf, 0, max_len);
    return 0;
#endif
}

uint32_t hal_system_get_cpu_freq(void)
{
    return sys_clock_hw_cycles_per_sec();
}

void hal_system_log_info(void)
{
    uint8_t uuid[8];
    int len = hal_system_get_uuid(uuid, sizeof(uuid));
    if (len == 8) {
        LOG_INF("Device UUID: %02X%02X%02X%02X%02X%02X%02X%02X",
                uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0]);
    }
    LOG_INF("CPU Frequency: %u MHz", hal_system_get_cpu_freq() / 1000000);
}
