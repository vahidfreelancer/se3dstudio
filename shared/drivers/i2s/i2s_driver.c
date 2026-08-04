#include "i2s_driver.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2s_driver, LOG_LEVEL_INF);

int i2s_drv_init(const struct device *dev)
{
    if (dev == NULL) {
        LOG_ERR("I2S device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(dev)) {
        LOG_ERR("I2S device %s is not ready", dev->name);
        return -ENODEV;
    }
    LOG_INF("I2S device %s initialized", dev->name);
    return 0;
}

int i2s_drv_configure(const struct device *dev, enum i2s_dir dir, const struct i2s_config *cfg)
{
    if (dev == NULL || cfg == NULL) {
        return -EINVAL;
    }
    int ret = i2s_configure(dev, dir, cfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S");
    }
    return ret;
}

int i2s_drv_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
    if (dev == NULL) {
        return -EINVAL;
    }
    int ret = i2s_trigger(dev, dir, cmd);
    if (ret < 0) {
        LOG_ERR("Failed to trigger I2S");
    }
    return ret;
}

int i2s_drv_read(const struct device *dev, void **buf, size_t *size)
{
    if (dev == NULL || buf == NULL || size == NULL) {
        return -EINVAL;
    }
    return i2s_read(dev, buf, size);
}

int i2s_drv_write(const struct device *dev, void *buf, size_t size)
{
    if (dev == NULL || buf == NULL) {
        return -EINVAL;
    }
    return i2s_write(dev, buf, size);
}
