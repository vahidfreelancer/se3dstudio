#include "hal_timer.h"

void hal_timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn, k_timer_stop_t stop_fn)
{
    if (timer != NULL) {
        k_timer_init(timer, expiry_fn, stop_fn);
    }
}

void hal_timer_start(struct k_timer *timer, int64_t duration_ms, int64_t period_ms)
{
    if (timer != NULL) {
        k_timer_start(timer, K_MSEC(duration_ms), K_MSEC(period_ms));
    }
}

void hal_timer_stop(struct k_timer *timer)
{
    if (timer != NULL) {
        k_timer_stop(timer);
    }
}

uint32_t hal_timer_get_remaining(struct k_timer *timer)
{
    if (timer == NULL) {
        return 0;
    }
    return (uint32_t)k_timer_remaining_get(timer);
}
