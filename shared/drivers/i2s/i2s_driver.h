#ifndef I2S_DRIVER_H
#define I2S_DRIVER_H

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the I2S driver.
 * 
 * @param dev Pointer to the I2S device structure.
 * @return int 0 on success, negative errno on failure.
 */
int i2s_drv_init(const struct device *dev);

/**
 * @brief Configure I2S device.
 * 
 * @param dev Pointer to the I2S device.
 * @param dir Direction (I2S_DIR_TX or I2S_DIR_RX).
 * @param cfg Pointer to the configuration structure.
 * @return int 0 on success, negative errno on failure.
 */
int i2s_drv_configure(const struct device *dev, enum i2s_dir dir, const struct i2s_config *cfg);

/**
 * @brief Send trigger command to the I2S device.
 * 
 * @param dev Pointer to the I2S device.
 * @param dir Direction (I2S_DIR_TX or I2S_DIR_RX).
 * @param cmd Trigger command (e.g. I2S_TRIGGER_START).
 * @return int 0 on success, negative errno on failure.
 */
int i2s_drv_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd);

/**
 * @brief Read I2S data block.
 * 
 * @param dev Pointer to the I2S device.
 * @param buf Pointer to pointer of buffer containing read data.
 * @param size Pointer to store buffer size.
 * @return int 0 on success, negative errno on failure.
 */
int i2s_drv_read(const struct device *dev, void **buf, size_t *size);

/**
 * @brief Write I2S data block.
 * 
 * @param dev Pointer to the I2S device.
 * @param buf Pointer to data buffer to write.
 * @param size Size of the data block in bytes.
 * @return int 0 on success, negative errno on failure.
 */
int i2s_drv_write(const struct device *dev, void *buf, size_t size);

#endif // I2S_DRIVER_H
