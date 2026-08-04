#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

/**
 * @brief Get the system uptime in seconds.
 * 
 * @return uint64_t System uptime in seconds.
 */
uint64_t utils_get_uptime_seconds(void);

/**
 * @brief Get the system uptime in milliseconds.
 * 
 * @return int64_t System uptime in milliseconds.
 */
int64_t utils_get_uptime_ms(void);

/**
 * @brief Log the system status summary (uptime, etc.) to logs.
 */
void utils_log_system_status(void);

#endif // UTILS_H
