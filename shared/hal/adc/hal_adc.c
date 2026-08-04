#include "hal_adc.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hal_adc, LOG_LEVEL_INF);

int hal_adc_init(const struct device *adc_dev)
{
    if (adc_dev == NULL) {
        LOG_ERR("ADC device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device %s is not ready", adc_dev->name);
        return -ENODEV;
    }
    return 0;
}

int hal_adc_channel_setup(const struct device *adc_dev, const struct adc_channel_cfg *cfg)
{
    if (adc_dev == NULL || cfg == NULL) {
        return -EINVAL;
    }
    int ret = adc_channel_setup(adc_dev, cfg);
    if (ret < 0) {
        LOG_ERR("Failed to setup ADC channel (err %d)", ret);
    }
    return ret;
}

int hal_adc_read_raw(const struct device *adc_dev, uint8_t channel_id, uint8_t resolution, int16_t *val)
{
    if (adc_dev == NULL || val == NULL) {
        return -EINVAL;
    }

    struct adc_sequence seq = {
        .channels = BIT(channel_id),
        .buffer = val,
        .buffer_size = sizeof(*val),
        .resolution = resolution,
    };

    int ret = adc_read(adc_dev, &seq);
    if (ret < 0) {
        LOG_ERR("Failed to read ADC raw on channel %d (err %d)", channel_id, ret);
    }
    return ret;
}

int hal_adc_read_mv(const struct device *adc_dev, uint8_t channel_id, uint8_t resolution, int32_t vref_mv, int32_t *val_mv)
{
    int16_t raw_val;
    int ret = hal_adc_read_raw(adc_dev, channel_id, resolution, &raw_val);
    if (ret < 0) {
        return ret;
    }

    // Manual conversion formula: (raw * vref) / (2 ^ resolution)
    *val_mv = ((int32_t)raw_val * vref_mv) / (1 << resolution);
    return 0;
}
