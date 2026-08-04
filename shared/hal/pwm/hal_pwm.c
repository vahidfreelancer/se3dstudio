#include "hal_pwm.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hal_pwm, LOG_LEVEL_INF);

int hal_pwm_set_period_and_duty(const struct pwm_dt_spec *spec, uint32_t period_ns, uint32_t duty_cycle_ns)
{
    if (spec == NULL) {
        return -EINVAL;
    }
    if (!pwm_is_ready_dt(spec)) {
        LOG_ERR("PWM device %s is not ready", spec->dev->name);
        return -ENODEV;
    }

    int ret = pwm_set_dt(spec, period_ns, duty_cycle_ns);
    if (ret < 0) {
        LOG_ERR("Failed to set PWM period %u ns, duty %u ns (err %d)", period_ns, duty_cycle_ns, ret);
    }
    return ret;
}

int hal_pwm_set_hz(const struct pwm_dt_spec *spec, uint32_t frequency_hz, uint32_t duty_cycle_percent)
{
    if (spec == NULL || duty_cycle_percent > 100) {
        return -EINVAL;
    }
    if (frequency_hz == 0) {
        return hal_pwm_set_period_and_duty(spec, 0, 0); // Stop PWM
    }

    uint64_t period_ns = 1000000000ULL / frequency_hz;
    uint64_t duty_cycle_ns = (period_ns * duty_cycle_percent) / 100ULL;

    return hal_pwm_set_period_and_duty(spec, (uint32_t)period_ns, (uint32_t)duty_cycle_ns);
}
