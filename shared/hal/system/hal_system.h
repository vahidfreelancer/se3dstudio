#ifndef HAL_SYSTEM_H
#define HAL_SYSTEM_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Reboot the system.
 */
void hal_system_reboot(void);

/**
 * @brief Retrieve a unique device hardware identifier (UUID).
 * 
 * @param id_buf Buffer to store the UUID.
 * @param max_len Maximum length of the buffer.
 * @return int The number of bytes written to the buffer, or negative error.
 */
int hal_system_get_uuid(uint8_t *id_buf, size_t max_len);

/**
 * @brief Get the CPU frequency in Hz.
 * 
 * @return uint32_t Clock frequency in Hz.
 */
uint32_t hal_system_get_cpu_freq(void);

/**
 * @brief Log system information (Reboot reason, HW details).
 */
void hal_system_log_info(void);

#endif // HAL_SYSTEM_H
