#include "imu_manager/imu_manager.h"

#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <semphr.h>
#include <task.h>

#include "logger/logger.h"
#include "main.h"
#include "mpu6050/mpu6050.h"
#include "queue_manager/queue_manager.h"

static SemaphoreHandle_t imu_data_ready_sem = NULL;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == INT_Pin) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (imu_data_ready_sem != NULL) {
            xSemaphoreGiveFromISR(imu_data_ready_sem, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void imu_task(void *argument) {
    (void)argument;
    while (1) {
        if (xSemaphoreTake(imu_data_ready_sem, portMAX_DELAY) == pdTRUE) {
            imu_data_t imu_data;
            imu_data.pitch     = Read_DMP_pitch();
            imu_data.pitch_dot = Read_DMP_pitch_dot();
            PITCH_QUEUE_OVERRIDE(&imu_data);
        }
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

    imu_data_ready_sem = xSemaphoreCreateBinary();
    if (imu_data_ready_sem != NULL) {
        xTaskCreate(imu_task, "imu_task", 512, NULL, osPriorityRealtime, NULL);
    }
}