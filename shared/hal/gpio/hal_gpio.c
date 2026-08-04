#include "hal_gpio.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hal_gpio, LOG_LEVEL_INF);

int hal_gpio_configure(const struct gpio_dt_spec *spec, gpio_flags_t extra_flags)
{
    if (spec == NULL) {
        return -EINVAL;
    }

    if (!gpio_is_ready_dt(spec)) {
        LOG_ERR("GPIO port %s is not ready", spec->port->name);
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(spec, spec->dt_flags | extra_flags);
    if (ret < 0) {
        LOG_ERR("Failed to configure GPIO pin %d (err %d)", spec->pin, ret);
    }
    return ret;
}

int hal_gpio_write(const struct gpio_dt_spec *spec, int value)
{
    if (spec == NULL) {
        return -EINVAL;
    }
    return gpio_pin_set_dt(spec, value);
}

int hal_gpio_read(const struct gpio_dt_spec *spec)
{
    if (spec == NULL) {
        return -EINVAL;
    }
    return gpio_pin_get_dt(spec);
}

int hal_gpio_toggle(const struct gpio_dt_spec *spec)
{
    if (spec == NULL) {
        return -EINVAL;
    }
    return gpio_pin_toggle_dt(spec);
}

int hal_gpio_configure_interrupt(const struct gpio_dt_spec *spec,
                                 gpio_flags_t flags,
                                 gpio_callback_handler_t handler,
                                 struct gpio_callback *cb)
{
    if (spec == NULL || cb == NULL || handler == NULL) {
        return -EINVAL;
    }

    int ret = gpio_pin_interrupt_configure_dt(spec, flags);
    if (ret < 0) {
        LOG_ERR("Failed to configure GPIO pin %d interrupt (err %d)", spec->pin, ret);
        return ret;
    }

    gpio_init_callback(cb, handler, BIT(spec->pin));
    ret = gpio_add_callback(spec->port, cb);
    if (ret < 0) {
        LOG_ERR("Failed to add GPIO callback (err %d)", ret);
    }
    return ret;
}
