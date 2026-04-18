
#include <SEGGER_RTT.h>
#include <stdio.h>
#include <string.h>

#include "cli/cli_rtt.h"
#include "cli_task/cli_commands.h"
#include "logger/logger.h"
#include "queue_manager/queue_manager.h"

static void cmd_help(int argc, char *argv[]) {
    LOG("Available commands:\r\n");
    uint8_t count = cli_get_command_count();
    for (uint8_t i = 0; i < count; i++) {
        const cli_command_t *cmd = cli_get_command_at_index(i);
        if (cmd) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "- %s: %s\r\n", cmd->command, cmd->description);
            LOG(buffer);
        }
    }
}

void cli_register_all_commands(void) {
    cli_register_command("help", cmd_help, "Show available commands");
}
