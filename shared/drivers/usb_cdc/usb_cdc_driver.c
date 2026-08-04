#include "usb_cdc_driver.h"
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb_cdc_driver, LOG_LEVEL_INF);

static const struct device *cdc_dev = NULL;

int usb_cdc_drv_init(void)
{
    int ret;

    #if DT_NODE_EXISTS(DT_CHOSEN(zephyr_cdc_acm_uart))
    cdc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_cdc_acm_uart));
    #else
    LOG_ERR("zephyr,cdc-acm-uart chosen node not found in devicetree");
    return -ENODEV;
    #endif

    if (!device_is_ready(cdc_dev)) {
        LOG_ERR("USB CDC ACM device %s is not ready", cdc_dev->name);
        return -ENODEV;
    }

    ret = usb_enable(NULL);
    if (ret != 0) {
        LOG_ERR("Failed to enable USB stack (err %d)", ret);
        return ret;
    }

    LOG_INF("USB CDC ACM driver initialized and stack enabled");
    return 0;
}

int usb_cdc_drv_tx(const uint8_t *data, size_t len)
{
    if (cdc_dev == NULL || data == NULL) {
        return -EINVAL;
    }

    for (size_t i = 0; i < len; i++) {
        uart_poll_out(cdc_dev, data[i]);
    }
    return 0;
}

int usb_cdc_drv_set_rx_callback(uart_irq_callback_user_data_t cb, void *user_data)
{
    if (cdc_dev == NULL) {
        return -EINVAL;
    }

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    uart_irq_callback_user_data_set(cdc_dev, cb, user_data);
    uart_irq_rx_enable(cdc_dev);
    return 0;
#else
    LOG_ERR("CONFIG_UART_INTERRUPT_DRIVEN is disabled; cannot set IRQ callback");
    return -ENOTSUP;
#endif
}
