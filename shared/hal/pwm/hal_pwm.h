#ifndef HAL_PWM_H
#define HAL_PWM_H

#include <zephyr/drivers/pwm.h>

/**
 * @brief Set the period and duty cycle (in nanoseconds) of a PWM spec.
 * 
 * @param spec Pointer to the PWM spec structure.
 * @param period_ns PWM period in nanoseconds.
 * @param duty_cycle_ns PWM duty cycle in nanoseconds.
 * @return int 0 on success, negative errno on failure.
 */
int hal_pwm_set_period_and_duty(const struct pwm_dt_spec *spec, uint32_t period_ns, uint32_t duty_cycle_ns);

/**
 * @brief Set PWM output by frequency and duty cycle percentage.
 * 
 * @param spec Pointer to the PWM spec structure.
 * @param frequency_hz PWM frequency in Hz.
 * @param duty_cycle_percent PWM duty cycle percentage (0-100).
 * @return int 0 on success, negative errno on failure.
 */
int hal_pwm_set_hz(const struct pwm_dt_spec *spec, uint32_t frequency_hz, uint32_t duty_cycle_percent);

#endif // HAL_PWM_H
