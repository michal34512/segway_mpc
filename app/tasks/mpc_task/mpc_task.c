#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <stdint.h>
#include <stm32h7xx_hal.h>
#include <task.h>

#include "logger/logger.h"
#include "mpc_task/mpc_task.h"
#include "queue_manager/queue_manager.h"


#define SENSORS_TASK_STACK_SIZE (512 * 4)
#if SENSORS_TASK_STACK_SIZE % 4 != 0
  #error "Task stack size must be divisible by 4!"
#endif

static osThreadId_t mpc_task_handle;
static uint32_t mpc_task_buffer[SENSORS_TASK_STACK_SIZE];
static StaticTask_t mpc_task_control_block;
static const osThreadAttr_t mpc_task_attributes = {
  .name       = "Sensors Task",
  .stack_mem  = &mpc_task_buffer,
  .stack_size = sizeof(mpc_task_buffer),
  .cb_mem     = &mpc_task_control_block,
  .cb_size    = sizeof(mpc_task_control_block),
  .priority   = (osPriority_t)osPriorityNormal,
};

static void mpc_task(void *argument) {
    (void)argument;

    while (1) {
        float pitch = 0;
        PITCH_QUEUE_PEEK(&pitch);
        LOG_INFO("Pitch: %5.2f\n\r", pitch);
        vTaskDelay(10);
    }
}

void mpc_task_init() {
    mpc_task_handle = osThreadNew(mpc_task, NULL, &mpc_task_attributes);
    assert_param(mpc_task_handle);
}
