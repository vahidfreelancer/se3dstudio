#include "hal_flash.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hal_flash, LOG_LEVEL_INF);

int hal_flash_init(const struct device *flash_dev)
{
    if (flash_dev == NULL) {
        LOG_ERR("Flash device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(flash_dev)) {
        LOG_ERR("Flash device %s is not ready", flash_dev->name);
        return -ENODEV;
    }
    return 0;
}

int hal_flash_read(const struct device *flash_dev, off_t offset, void *data, size_t len)
{
    if (flash_dev == NULL || data == NULL) {
        return -EINVAL;
    }
    int ret = flash_read(flash_dev, offset, data, len);
    if (ret < 0) {
        LOG_ERR("Failed to read flash at offset 0x%lx, len %zu (err %d)", (long)offset, len, ret);
    }
    return ret;
}

int hal_flash_write(const struct device *flash_dev, off_t offset, const void *data, size_t len)
{
    if (flash_dev == NULL || data == NULL) {
        return -EINVAL;
    }
    int ret = flash_write(flash_dev, offset, data, len);
    if (ret < 0) {
        LOG_ERR("Failed to write flash at offset 0x%lx, len %zu (err %d)", (long)offset, len, ret);
    }
    return ret;
}

int hal_flash_erase(const struct device *flash_dev, off_t offset, size_t size)
{
    if (flash_dev == NULL) {
        return -EINVAL;
    }
    int ret = flash_erase(flash_dev, offset, size);
    if (ret < 0) {
        LOG_ERR("Failed to erase flash at offset 0x%lx, size %zu (err %d)", (long)offset, size, ret);
    }
    return ret;
}

int hal_flash_get_page_info(const struct device *flash_dev, off_t offset, struct flash_pages_info *page_info)
{
    if (flash_dev == NULL || page_info == NULL) {
        return -EINVAL;
    }
    return flash_get_page_info_by_offs(flash_dev, offset, page_info);
}
