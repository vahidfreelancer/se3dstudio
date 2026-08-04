#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <zephyr/drivers/adc.h>

/**
 * @brief Initialize the ADC driver.
 * 
 * @param adc_dev Pointer to the ADC device structure.
 * @return int 0 on success, negative errno on failure.
 */
int hal_adc_init(const struct device *adc_dev);

/**
 * @brief Set up a channel on the ADC device.
 * 
 * @param adc_dev Pointer to the ADC device structure.
 * @param cfg Pointer to the channel configuration structure.
 * @return int 0 on success, negative errno on failure.
 */
int hal_adc_channel_setup(const struct device *adc_dev, const struct adc_channel_cfg *cfg);

/**
 * @brief Read raw value from an ADC channel.
 * 
 * @param adc_dev Pointer to the ADC device structure.
 * @param channel_id The channel number to read from.
 * @param resolution The resolution in bits (e.g., 10 or 12).
 * @param val Pointer to store the raw reading.
 * @return int 0 on success, negative errno on failure.
 */
int hal_adc_read_raw(const struct device *adc_dev, uint8_t channel_id, uint8_t resolution, int16_t *val);

/**
 * @brief Read value in millivolts from an ADC channel.
 * 
 * @param adc_dev Pointer to the ADC device structure.
 * @param channel_id The channel number to read from.
 * @param resolution The resolution in bits (e.g., 10 or 12).
 * @param vref_mv Reference voltage in millivolts.
 * @param val_mv Pointer to store the reading in millivolts.
 * @return int 0 on success, negative errno on failure.
 */
int hal_adc_read_mv(const struct device *adc_dev, uint8_t channel_id, uint8_t resolution, int32_t vref_mv, int32_t *val_mv);

#endif // HAL_ADC_H
