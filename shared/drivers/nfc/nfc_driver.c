#include "nfc_driver.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#ifdef CONFIG_NFC_T2T_LIB
#include <nfc_t2t_lib.h>
#include <nfc/ndef/uri_msg.h>
#endif

LOG_MODULE_REGISTER(nfc_driver, LOG_LEVEL_INF);

#ifdef CONFIG_NFC_T2T_LIB
static uint8_t ndef_msg_buf[1024];

static void nfc_callback(void *context, nfc_t2t_event_t event, const uint8_t *data, size_t data_length)
{
    switch (event) {
    case NFC_T2T_EVENT_FIELD_ON:
        LOG_INF("NFC Field Detected (On)");
        break;
    case NFC_T2T_EVENT_FIELD_OFF:
        LOG_INF("NFC Field Lost (Off)");
        break;
    case NFC_T2T_EVENT_DATA_READ:
        LOG_INF("NFC Data Read");
        break;
    default:
        break;
    }
}
#endif

int nfc_drv_init(void)
{
#ifdef CONFIG_NFC_T2T_LIB
    int ret = nfc_t2t_setup(nfc_callback, NULL);
    if (ret < 0) {
        LOG_ERR("Failed to setup NFC T2T library (err %d)", ret);
        return ret;
    }
    LOG_INF("NFC Type 2 Tag emulation initialized");
    return 0;
#else
    LOG_WRN("NFC T2T emulation library not compiled (CONFIG_NFC_T2T_LIB is disabled)");
    return -ENOTSUP;
#endif
}

int nfc_drv_set_uri(const char *uri)
{
#ifdef CONFIG_NFC_T2T_LIB
    if (uri == NULL) {
        return -EINVAL;
    }

    uint32_t len = sizeof(ndef_msg_buf);
    int ret = nfc_ndef_uri_msg_create(NFC_NDEF_URI_REC_TYPE_HTTPS,
                                      (const uint8_t *)uri,
                                      strlen(uri),
                                      ndef_msg_buf,
                                      &len);
    if (ret < 0) {
        LOG_ERR("Failed to create NDEF URI message (err %d)", ret);
        return ret;
    }

    ret = nfc_t2t_payload_set(ndef_msg_buf, len);
    if (ret < 0) {
        LOG_ERR("Failed to set NFC T2T payload (err %d)", ret);
        return ret;
    }

    LOG_INF("NFC NDEF URI payload set: %s (size %u)", uri, len);
    return 0;
#else
    LOG_WRN("NFC set URI not supported; library not enabled");
    return -ENOTSUP;
#endif
}

int nfc_drv_start(void)
{
#ifdef CONFIG_NFC_T2T_LIB
    int ret = nfc_t2t_emulation_start();
    if (ret < 0) {
        LOG_ERR("Failed to start NFC emulation (err %d)", ret);
        return ret;
    }
    LOG_INF("NFC emulation started");
    return 0;
#else
    LOG_WRN("NFC start not supported; library not enabled");
    return -ENOTSUP;
#endif
}

int nfc_drv_stop(void)
{
#ifdef CONFIG_NFC_T2T_LIB
    int ret = nfc_t2t_emulation_stop();
    if (ret < 0) {
        LOG_ERR("Failed to stop NFC emulation (err %d)", ret);
        return ret;
    }
    LOG_INF("NFC emulation stopped");
    return 0;
#else
    LOG_WRN("NFC stop not supported; library not enabled");
    return -ENOTSUP;
#endif
}
