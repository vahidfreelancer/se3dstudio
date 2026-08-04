#include "hal_rng.h"
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hal_rng, LOG_LEVEL_INF);

int hal_rng_init(const struct device *entropy_dev)
{
    if (entropy_dev == NULL) {
        LOG_ERR("Entropy device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(entropy_dev)) {
        LOG_ERR("Entropy device %s is not ready", entropy_dev->name);
        return -ENODEV;
    }
    return 0;
}

int hal_rng_get_bytes(const struct device *entropy_dev, uint8_t *buffer, size_t len)
{
    if (entropy_dev == NULL || buffer == NULL) {
        return -EINVAL;
    }
    int ret = entropy_get_entropy(entropy_dev, buffer, len);
    if (ret < 0) {
        LOG_ERR("Failed to get entropy (err %d)", ret);
    }
    return ret;
}

uint32_t hal_rng_get_u32(void)
{
    return sys_rand32_get();
}
