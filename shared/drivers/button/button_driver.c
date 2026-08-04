#include "button_driver.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(button_driver, LOG_LEVEL_INF);

int button_drv_init(const struct gpio_dt_spec *spec,
                    gpio_callback_handler_t handler,
                    struct gpio_callback *cb)
{
    if (spec == NULL || handler == NULL || cb == NULL) {
        return -EINVAL;
    }

    if (!gpio_is_ready_dt(spec)) {
        LOG_ERR("Button GPIO port %s is not ready", spec->port->name);
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(spec, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure button GPIO pin %d (err %d)", spec->pin, ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure interrupt on button pin %d (err %d)", spec->pin, ret);
        return ret;
    }

    gpio_init_callback(cb, handler, BIT(spec->pin));
    ret = gpio_add_callback(spec->port, cb);
    if (ret < 0) {
        LOG_ERR("Failed to add GPIO callback for button (err %d)", ret);
    }
    return ret;
}

int button_drv_read(const struct gpio_dt_spec *spec)
{
    if (spec == NULL) {
        return -EINVAL;
    }
    return gpio_pin_get_dt(spec);
}
