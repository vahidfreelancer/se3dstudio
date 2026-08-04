#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the I2C driver.
 * 
 * @param dev Pointer to the I2C device structure.
 * @return int 0 on success, negative errno on failure.
 */
int i2c_drv_init(const struct device *dev);

/**
 * @brief Write data to an I2C device.
 * 
 * @param dev Pointer to the I2C device structure.
 * @param buf Data buffer to write.
 * @param num_bytes Length of data to write.
 * @param addr I2C address of target device.
 * @return int 0 on success, negative errno on failure.
 */
int i2c_drv_write(const struct device *dev, const uint8_t *buf, uint32_t num_bytes, uint16_t addr);

/**
 * @brief Read data from an I2C device.
 * 
 * @param dev Pointer to the I2C device structure.
 * @param buf Data buffer to store read values.
 * @param num_bytes Length of data to read.
 * @param addr I2C address of target device.
 * @return int 0 on success, negative errno on failure.
 */
int i2c_drv_read(const struct device *dev, uint8_t *buf, uint32_t num_bytes, uint16_t addr);

/**
 * @brief Write data and then read data in a single transaction (Repeated Start).
 * 
 * @param dev Pointer to the I2C device structure.
 * @param addr I2C address of target device.
 * @param write_buf Data buffer to write.
 * @param num_write Length of write data.
 * @param read_buf Data buffer to store read values.
 * @param num_read Length of read data.
 * @return int 0 on success, negative errno on failure.
 */
int i2c_drv_write_read(const struct device *dev, uint16_t addr,
                       const void *write_buf, size_t num_write,
                       void *read_buf, size_t num_read);

#endif // I2C_DRIVER_H
