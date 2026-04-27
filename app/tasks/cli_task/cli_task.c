#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <stdint.h>
#include <stm32h7xx_hal.h>
#include <task.h>

#include "logger/logger.h"

#include "cli/cli_rtt.h"
#include "cli_task/cli_commands.h"
#include "cli_task/cli_task.h"

#define CLI_TASK_STACK_SIZE (256 * 4)
#if CLI_TASK_STACK_SIZE % 4 != 0
  #error "Task stack size must be divisible by 4!"
#endif

static osThreadId_t cli_task_handle;
static uint32_t cli_task_buffer[CLI_TASK_STACK_SIZE];
static StaticTask_t cli_task_control_block;
static const osThreadAttr_t cli_task_attributes = {
  .name       = "CliTask",
  .stack_mem  = &cli_task_buffer,
  .stack_size = sizeof(cli_task_buffer),
  .cb_mem     = &cli_task_control_block,
  .cb_size    = sizeof(cli_task_control_block),
  .priority   = (osPriority_t)osPriorityNormal,
};

static void cli_task(void *argument) {
    (void)argument;
    LOG_DEBUG("Cli task started\r\n");

    cli_init();
    cli_register_all_commands();

    while (1) {
        cli_process();
        osDelay(10);  // 100Hz
    }
}

void cli_task_init() {
    cli_task_handle = osThreadNew(cli_task, NULL, &cli_task_attributes);
    assert_param(cli_task_handle);
}
