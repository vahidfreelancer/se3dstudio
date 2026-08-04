#include "i2c_driver.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_driver, LOG_LEVEL_INF);

int i2c_drv_init(const struct device *dev)
{
    if (dev == NULL) {
        LOG_ERR("I2C device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(dev)) {
        LOG_ERR("I2C device %s is not ready", dev->name);
        return -ENODEV;
    }
    LOG_INF("I2C device %s initialized", dev->name);
    return 0;
}

int i2c_drv_write(const struct device *dev, const uint8_t *buf, uint32_t num_bytes, uint16_t addr)
{
    if (dev == NULL || buf == NULL) {
        return -EINVAL;
    }
    return i2c_write(dev, buf, num_bytes, addr);
}

int i2c_drv_read(const struct device *dev, uint8_t *buf, uint32_t num_bytes, uint16_t addr)
{
    if (dev == NULL || buf == NULL) {
        return -EINVAL;
    }
    return i2c_read(dev, buf, num_bytes, addr);
}

int i2c_drv_write_read(const struct device *dev, uint16_t addr,
                       const void *write_buf, size_t num_write,
                       void *read_buf, size_t num_read)
{
    if (dev == NULL) {
        return -EINVAL;
    }
    return i2c_write_read(dev, addr, write_buf, num_write, read_buf, num_read);
}
