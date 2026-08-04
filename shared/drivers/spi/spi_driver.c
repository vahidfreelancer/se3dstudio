#include "spi_driver.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_driver, LOG_LEVEL_INF);

int spi_drv_init(const struct device *dev)
{
    if (dev == NULL) {
        LOG_ERR("SPI device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(dev)) {
        LOG_ERR("SPI device %s is not ready", dev->name);
        return -ENODEV;
    }
    LOG_INF("SPI device %s initialized", dev->name);
    return 0;
}

int spi_drv_transceive(const struct device *dev,
                       const struct spi_config *config,
                       const struct spi_buf_set *tx_bufs,
                       const struct spi_buf_set *rx_bufs)
{
    if (dev == NULL || config == NULL) {
        return -EINVAL;
    }
    return spi_transceive(dev, config, tx_bufs, rx_bufs);
}

int spi_drv_write(const struct device *dev,
                  const struct spi_config *config,
                  const uint8_t *data,
                  size_t len)
{
    struct spi_buf tx_buf = {
        .buf = (void *)data,
        .len = len,
    };
    struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1,
    };
    return spi_drv_transceive(dev, config, &tx_bufs, NULL);
}

int spi_drv_read(const struct device *dev,
                 const struct spi_config *config,
                 uint8_t *data,
                 size_t len)
{
    struct spi_buf rx_buf = {
        .buf = data,
        .len = len,
    };
    struct spi_buf_set rx_bufs = {
        .buffers = &rx_buf,
        .count = 1,
    };
    return spi_drv_transceive(dev, config, NULL, &rx_bufs);
}
