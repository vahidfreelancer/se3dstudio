#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize a system timer.
 * 
 * @param timer Pointer to the kernel timer structure.
 * @param expiry_fn Expiry callback function.
 * @param stop_fn Stop callback function.
 */
void hal_timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn, k_timer_stop_t stop_fn);

/**
 * @brief Start a system timer.
 * 
 * @param timer Pointer to the kernel timer structure.
 * @param duration_ms Duration before first expiry (in milliseconds).
 * @param period_ms Periodic interval (in milliseconds, 0 for one-shot).
 */
void hal_timer_start(struct k_timer *timer, int64_t duration_ms, int64_t period_ms);

/**
 * @brief Stop a running timer.
 * 
 * @param timer Pointer to the kernel timer structure.
 */
void hal_timer_stop(struct k_timer *timer);

/**
 * @brief Get the remaining time before the next expiry.
 * 
 * @param timer Pointer to the kernel timer structure.
 * @return uint32_t Remaining time in milliseconds.
 */
uint32_t hal_timer_get_remaining(struct k_timer *timer);

#endif // HAL_TIMER_H
