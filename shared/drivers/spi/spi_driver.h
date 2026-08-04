#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the SPI driver.
 * 
 * @param dev Pointer to the SPI device structure.
 * @return int 0 on success, negative errno on failure.
 */
int spi_drv_init(const struct device *dev);

/**
 * @brief Synchronously transceive buffers over SPI.
 * 
 * @param dev Pointer to the SPI device structure.
 * @param config SPI configuration struct.
 * @param tx_bufs Set of transmit buffers.
 * @param rx_bufs Set of receive buffers.
 * @return int 0 on success, negative errno on failure.
 */
int spi_drv_transceive(const struct device *dev,
                       const struct spi_config *config,
                       const struct spi_buf_set *tx_bufs,
                       const struct spi_buf_set *rx_bufs);

/**
 * @brief Simple SPI write helper.
 * 
 * @param dev Pointer to the SPI device structure.
 * @param config SPI configuration struct.
 * @param data Data buffer to write.
 * @param len Length of data in bytes.
 * @return int 0 on success, negative errno on failure.
 */
int spi_drv_write(const struct device *dev,
                  const struct spi_config *config,
                  const uint8_t *data,
                  size_t len);

/**
 * @brief Simple SPI read helper.
 * 
 * @param dev Pointer to the SPI device structure.
 * @param config SPI configuration struct.
 * @param data Data buffer to store read values.
 * @param len Length of data in bytes.
 * @return int 0 on success, negative errno on failure.
 */
int spi_drv_read(const struct device *dev,
                 const struct spi_config *config,
                 uint8_t *data,
                 size_t len);

#endif // SPI_DRIVER_H
