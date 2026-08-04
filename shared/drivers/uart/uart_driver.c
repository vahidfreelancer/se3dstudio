#include "uart_driver.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_driver, LOG_LEVEL_INF);

int uart_drv_init(const struct device *dev, uart_irq_callback_user_data_t cb, void *user_data)
{
    if (dev == NULL) {
        LOG_ERR("UART device pointer is NULL");
        return -EINVAL;
    }
    if (!device_is_ready(dev)) {
        LOG_ERR("UART device %s is not ready", dev->name);
        return -ENODEV;
    }

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    if (cb != NULL) {
        uart_irq_callback_user_data_set(dev, cb, user_data);
    }
#else
    if (cb != NULL) {
        LOG_WRN("UART interrupts are disabled (CONFIG_UART_INTERRUPT_DRIVEN is not set)");
    }
#endif

    LOG_INF("UART device %s initialized", dev->name);
    return 0;
}

int uart_drv_tx(const struct device *dev, const uint8_t *data, size_t len)
{
    if (dev == NULL || data == NULL) {
        return -EINVAL;
    }

    for (size_t i = 0; i < len; i++) {
        uart_poll_out(dev, data[i]);
    }
    return 0;
}

int uart_drv_rx_enable(const struct device *dev)
{
    if (dev == NULL) {
        return -EINVAL;
    }

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    uart_irq_rx_enable(dev);
    return 0;
#else
    LOG_ERR("Interrupt RX not supported (CONFIG_UART_INTERRUPT_DRIVEN is disabled)");
    return -ENOTSUP;
#endif
}

int uart_drv_rx_disable(const struct device *dev)
{
    if (dev == NULL) {
        return -EINVAL;
    }

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    uart_irq_rx_disable(dev);
    return 0;
#else
    LOG_ERR("Interrupt RX not supported (CONFIG_UART_INTERRUPT_DRIVEN is disabled)");
    return -ENOTSUP;
#endif
}
