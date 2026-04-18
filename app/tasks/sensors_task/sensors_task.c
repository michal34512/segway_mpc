#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <stdint.h>
#include <stm32h7xx_hal.h>
#include <task.h>

#include "sensors_task/sensors_task.h"

#include "logger/logger.h"
#include "sensors_task/sensors_manager.h"

#define SENSORS_TASK_STACK_SIZE (512 * 4)
#if SENSORS_TASK_STACK_SIZE % 4 != 0
  #error "Task stack size must be divisible by 4!"
#endif

static osThreadId_t sensors_task_handle;
static uint32_t sensors_task_buffer[SENSORS_TASK_STACK_SIZE];
static StaticTask_t sensors_task_control_block;
static const osThreadAttr_t sensors_task_attributes = {
  .name       = "Sensors Task",
  .stack_mem  = &sensors_task_buffer,
  .stack_size = sizeof(sensors_task_buffer),
  .cb_mem     = &sensors_task_control_block,
  .cb_size    = sizeof(sensors_task_control_block),
  .priority   = (osPriority_t)osPriorityNormal,
};

static void sensors_task(void *argument) {
    (void)argument;
    LOG_DEBUG("Sensors task started\r\n");
    sensors_manager_init();
    while (1) {
        sensors_manager_process();
    }
}

void sensors_task_init() {
    sensors_task_handle = osThreadNew(sensors_task, NULL, &sensors_task_attributes);
    assert_param(sensors_task_handle);
}
