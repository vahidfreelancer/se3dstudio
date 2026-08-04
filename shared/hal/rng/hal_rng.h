#ifndef HAL_RNG_H
#define HAL_RNG_H

#include <zephyr/device.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the hardware entropy (RNG) device.
 * 
 * @param entropy_dev Pointer to the entropy device.
 * @return int 0 on success, negative errno on failure.
 */
int hal_rng_init(const struct device *entropy_dev);

/**
 * @brief Get random bytes from the entropy source.
 * 
 * @param entropy_dev Pointer to the entropy source device.
 * @param buffer Buffer to fill with random data.
 * @param len Number of bytes to fetch.
 * @return int 0 on success, negative errno on failure.
 */
int hal_rng_get_bytes(const struct device *entropy_dev, uint8_t *buffer, size_t len);

/**
 * @brief Get a cryptographically secure 32-bit random number.
 * 
 * @return uint32_t A random 32-bit integer.
 */
uint32_t hal_rng_get_u32(void);

#endif // HAL_RNG_H
