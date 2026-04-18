#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <stdint.h>
#include <stdio.h>
#include <stm32h725xx.h>
#include <task.h>
// #include <tim.h>

#include "cli_task/cli_task.h"
#include "logger/logger.h"
#include "main.h"
#include "queue_manager/queue_manager.h"
#include "sensors_task/sensors_task.h"
#include "sysview/sysview_macros.h"

void app_main(void *argument) {
    (void)argument;
    cli_task_init();
    queue_manager_init();
    SYSVIEW_START();

    while (1) {
        LOG_INFO("Hello RocketLab!\n");
        vTaskDelay(1000);
    }
    vTaskDelete(NULL);
}