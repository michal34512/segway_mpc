#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <stdint.h>
#include <stdio.h>
#include <stm32h725xx.h>
#include <task.h>
// #include <tim.h>
#include <string.h>

#include "cli_task/cli_task.h"
#include "logger/logger.h"
#include "main.h"
#include "queue_manager/queue_manager.h"
#include "sensors_task/sensors_task.h"
#include "sysview/sysview_macros.h"

#include "i2c_manager/i2c_manager.h"
#include "mpu6050/mpu6050.h"

void app_main(void *argument) {
    (void)argument;
    cli_task_init();
    queue_manager_init();
    SYSVIEW_START();

    while (!MPU6050_testConnection()) {
        LOG_INFO("BLAD: Brak komunikacji I2C z MPU6050!\n");
        vTaskDelay(1000);
    }
    LOG_INFO("MPU6050 polaczony poprawnie.\n");

    DMP_Init();  // Uruchomienie driverów InvSense i załadowanie firmware DMP

    while (1) {
        Read_DMP();  // Pobierz nowe dane z FIFO z DMP

        LOG_INFO("Pitch: %5.2f | A: X:%d Y:%d Z:%d | G: X:%d Y:%d Z:%d\n",
                 (double)Pitch,
                 accel[0],
                 accel[1],
                 accel[2],
                 gyro[0],
                 gyro[1],
                 gyro[2]);

        vTaskDelay(pdMS_TO_TICKS(10));  // Czekaj około 100 ms między odczytami
    }
    vTaskDelete(NULL);
}