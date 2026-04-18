#ifndef APP_DRIVERS_CLI_CLI_RTT_H_
#define APP_DRIVERS_CLI_CLI_RTT_H_

#include <stdint.h>

#define CLI_MAX_COMMAND_LENGTH 64
#define CLI_MAX_ARGUMENTS      8

typedef void (*cli_command_func_t)(int argc, char *argv[]);

typedef struct {
    const char *command;        /**< Command name */
    cli_command_func_t handler; /**< Handler function */
    const char *description;    /**< Short command description */
} cli_command_t;

void cli_init(void);

void cli_process(void);

void cli_register_command(const char *command, cli_command_func_t handler, const char *description);

uint8_t cli_get_command_count(void);

const cli_command_t *cli_get_command_at_index(uint8_t index);

#endif  // APP_DRIVERS_CLI_CLI_RTT_H_
