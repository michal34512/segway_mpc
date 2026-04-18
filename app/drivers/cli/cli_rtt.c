#include <SEGGER_RTT.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli/cli_rtt.h"
#include "logger/logger.h"

#define CLI_MAX_COMMANDS 32

static cli_command_t command_list[CLI_MAX_COMMANDS];
static uint8_t command_count = 0;
static char input_buffer[CLI_MAX_COMMAND_LENGTH];
static uint16_t input_pos = 0;

static int parse_arguments(char *input, char *argv[]) {
    int argc    = 0;
    char *token = strtok_r(input, " ", &input);
    while (token && (argc < CLI_MAX_ARGUMENTS)) {
        argv[argc++] = token;
        token        = strtok_r(NULL, " ", &input);
    }
    return argc;
}

void cli_register_command(const char *command, cli_command_func_t handler, const char *description) {
    if (command && handler && (command_count < CLI_MAX_COMMANDS)) {
        command_list[command_count].command     = command;
        command_list[command_count].handler     = handler;
        command_list[command_count].description = description;
        command_count++;
    }
}

uint8_t cli_get_command_count(void) {
    return command_count;
}

const cli_command_t *cli_get_command_at_index(uint8_t index) {
    return (index < command_count) ? &command_list[index] : NULL;
}

void cli_init(void) {
    logger_init();
    LOG("CLI initialized. Type 'help' for a list of commands.\r\n> ");
}

void cli_process(void) {
    int ch = SEGGER_RTT_GetKey();
    if (ch < 0) {
        return;
    }

    if ((ch == '\r') || (ch == '\n')) {
        input_buffer[input_pos] = '\0';
        if (input_pos > 0) {
            char *argv[CLI_MAX_ARGUMENTS];
            int argc = parse_arguments(input_buffer, argv);
            if (argc > 0) {
                uint8_t found = 0;
                for (uint8_t i = 0; i < command_count; i++) {
                    if (strcmp(argv[0], command_list[i].command) == 0) {
                        command_list[i].handler(argc, argv);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    LOG("Unknown command. Type 'help' for a list of commands.\r\n");
                }
            }
            input_pos = 0;
        }
        LOG(0, "> ");
    } else {
        if (input_pos < (CLI_MAX_COMMAND_LENGTH - 1)) {
            input_buffer[input_pos++] = (char)ch;
        }
    }
}
