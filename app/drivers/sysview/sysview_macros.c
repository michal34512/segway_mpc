#include <inttypes.h>
#include <global_timer.h>

#include "sysview/sysview_macros.h"

uint32_t SEGGER_SYSVIEW_X_GetTimestamp(void) {
    return (uint32_t)global_timer_get_us();
}
