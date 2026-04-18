#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <stdint.h>
#include <stdio.h>
#include <stm32h725xx.h>
#include <task.h>
// #include <tim.h>
#include <string.h>

#include "cli_task/cli_task.h"
#include "imu_manager/imu_manager.h"
#include "logger/logger.h"
#include "main.h"
#include "mpc_task/mpc_task.h"
#include "queue_manager/queue_manager.h"
#include "sysview/sysview_macros.h"

void app_main(void *argument) {
    (void)argument;
    queue_manager_init();
    cli_task_init();
    imu_manager_init();
    mpc_task_init();

    SYSVIEW_START();
    vTaskDelete(NULL);
}