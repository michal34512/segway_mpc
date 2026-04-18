#include "imu_manager/imu_manager.h"

#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <task.h>

#include "logger/logger.h"
#include "main.h"
#include "mpu6050/mpu6050.h"
#include "queue_manager/queue_manager.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == INT_Pin) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        float pitch                         = Read_DMP_pitch();
        PITCH_QUEUE_OVERRIDE_ISR(&pitch, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void imu_manager_init() {
    LOG_INFO("MPU6050 starting..\n");
    while (!MPU6050_testConnection()) {
        LOG_INFO("MPU6050 connection fail, retrying...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    LOG_INFO("MPU6050 connected!\n");
    DMP_Init();
    LOG_INFO("MPU6050 DMP initialized.\n");
}