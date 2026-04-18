#ifndef APP_LOGGER_H_
#define APP_LOGGER_H_

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE,
} LogLevel;

#if LOGGER_USE_RTT
void logger_init(void);
void log_message(LogLevel level, const char *filename, const char *format, ...);
#else
  #define logger_init() ((void)sizeof(0))
  #define log_message(level, filename, format, ...) ((void)sizeof(0))
#endif

// Macros for logging
#define LOG_DEBUG(format, ...) log_message(LOG_LEVEL_DEBUG, __FILE_NAME__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  log_message(LOG_LEVEL_INFO, __FILE_NAME__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  log_message(LOG_LEVEL_WARN, __FILE_NAME__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) log_message(LOG_LEVEL_ERROR, __FILE_NAME__, format, ##__VA_ARGS__)
#define LOG(format, ...)       log_message(LOG_LEVEL_NONE, __FILE_NAME__, format, ##__VA_ARGS__)

#endif  // APP_LOGGER_H_
