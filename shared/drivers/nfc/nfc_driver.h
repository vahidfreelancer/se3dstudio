#ifndef NFC_DRIVER_H
#define NFC_DRIVER_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize the NFC Type 2 Tag emulation library.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int nfc_drv_init(void);

/**
 * @brief Set the NDEF URI record payload.
 * 
 * @param uri The URI string (e.g. "www.seeedstudio.com").
 * @return int 0 on success, negative errno on failure.
 */
int nfc_drv_set_uri(const char *uri);

/**
 * @brief Start NFC tag emulation.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int nfc_drv_start(void);

/**
 * @brief Stop NFC tag emulation.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int nfc_drv_stop(void);

#endif // NFC_DRIVER_H
