#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <SEGGER_RTT.h>
#include <global_timer.h>

#include "logger/logger.h"

#define LOGGER_TIMEOUT_MS  1000
#define LOGGER_BUFFER_SIZE 256
#define RTT_TERMINAL_ID 0

#if LOGGER_USE_RTT
void logger_init(void) {
    // Initialize RTT
    SEGGER_RTT_SetTerminal(RTT_TERMINAL_ID);
}

void log_message(LogLevel level, const char *filename, const char *format, ...) {
    char buffer[LOGGER_BUFFER_SIZE] = {0};
    memset(buffer, 0, sizeof(buffer));
    const char *level_str;

    switch (level) {
    case LOG_LEVEL_DEBUG:
        level_str = "DEBUG";
        break;
    case LOG_LEVEL_INFO:
        level_str = "INFO";
        break;
    case LOG_LEVEL_WARN:
        level_str = "WARN";
        break;
    case LOG_LEVEL_ERROR:
        level_str = "ERROR";
        break;
    default:
        level_str = "";
        break;
    }

    uint64_t timestamp = global_timer_get_us();

    int16_t prefix_size = 0;
    if (level != LOG_LEVEL_NONE) {
        uint32_t first_half = timestamp / 1000000000;
        if (first_half == 0) {
            prefix_size = snprintf(buffer, sizeof(buffer), "[%s][%s][%lu] ", filename, level_str, (uint32_t)timestamp);
        } else {
            prefix_size = snprintf(buffer,
                                   sizeof(buffer),
                                   "[%s][%s][%lu%09lu] ",
                                   filename,
                                   level_str,
                                   first_half,
                                   (uint32_t)(timestamp % 1000000000));
        }
        if (prefix_size < 0 || prefix_size >= (int)sizeof(buffer))
            return;
    }

    va_list args;
    va_start(args, format);
    int16_t message_size = vsnprintf(buffer + prefix_size, sizeof(buffer) - prefix_size, format, args);
    va_end(args);

    if (message_size < 0 || (prefix_size + message_size) >= (int)sizeof(buffer)) {
        return;
    }

    SEGGER_RTT_SetTerminal(RTT_TERMINAL_ID);
    SEGGER_RTT_printf(0, "%s", buffer);
}
#endif
