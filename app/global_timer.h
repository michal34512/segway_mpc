#ifndef APP_GLOBAL_TIMER_H_
#define APP_GLOBAL_TIMER_H_

#include <stdint.h>

#include "cmsis_os.h"

static inline uint64_t global_timer_get_us(void) {
    TickType_t ticks = xTaskGetTickCount();
    return (uint64_t)ticks * (1000000ULL / configTICK_RATE_HZ);
}

static inline uint32_t global_timer_get_time_ms(void) {
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static inline uint64_t global_timer_get_interval(uint64_t t1, uint64_t t2) {
    return (t1 >= t2) ? (t1 - t2) : (UINT64_MAX - t2 + t1 + 1ULL);
}

#endif  // APP_GLOBAL_TIMER_H_
