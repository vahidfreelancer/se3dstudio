#ifndef HAL_FLASH_H
#define HAL_FLASH_H

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Initialize the flash device.
 * 
 * @param flash_dev Pointer to the flash device.
 * @return int 0 on success, negative errno on failure.
 */
int hal_flash_init(const struct device *flash_dev);

/**
 * @brief Read data from flash memory.
 * 
 * @param flash_dev Pointer to the flash device.
 * @param offset Offset from the start of flash to read.
 * @param data Buffer to store read data.
 * @param len Length of data to read in bytes.
 * @return int 0 on success, negative errno on failure.
 */
int hal_flash_read(const struct device *flash_dev, off_t offset, void *data, size_t len);

/**
 * @brief Write data to flash memory.
 * 
 * @param flash_dev Pointer to the flash device.
 * @param offset Offset from the start of flash to write.
 * @param data Buffer containing data to write.
 * @param len Length of data to write in bytes.
 * @return int 0 on success, negative errno on failure.
 */
int hal_flash_write(const struct device *flash_dev, off_t offset, const void *data, size_t len);

/**
 * @brief Erase a sector or range in flash memory.
 * 
 * @param flash_dev Pointer to the flash device.
 * @param offset Offset from the start of flash to erase.
 * @param size Size to erase in bytes (must be aligned to sector size).
 * @return int 0 on success, negative errno on failure.
 */
int hal_flash_erase(const struct device *flash_dev, off_t offset, size_t size);

/**
 * @brief Get Flash sector layout page information.
 * 
 * @param flash_dev Pointer to the flash device.
 * @param offset Offset in flash.
 * @param page_info Structure to store the page information.
 * @return int 0 on success, negative errno on failure.
 */
int hal_flash_get_page_info(const struct device *flash_dev, off_t offset, struct flash_pages_info *page_info);

#endif // HAL_FLASH_H
