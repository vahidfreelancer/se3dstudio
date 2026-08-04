#include "utils.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(utils, LOG_LEVEL_INF);

uint64_t utils_get_uptime_seconds(void)
{
    return (uint64_t)(k_uptime_get() / 1000);
}

int64_t utils_get_uptime_ms(void)
{
    return k_uptime_get();
}

void utils_log_system_status(void)
{
    uint64_t uptime_sec = utils_get_uptime_seconds();
    LOG_INF("System Uptime: %llu seconds", uptime_sec);
}
