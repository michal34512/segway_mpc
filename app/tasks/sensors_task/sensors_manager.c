#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <stdbool.h>
#include <stdint.h>
#include <task.h>

#include "sensors_task/sensors_manager.h"

#include "common/common_data.h"
#include "logger/logger.h"
#include "stopwatch/stopwatch.h"

#include "bmi088/bmi088_hal.h"
#include "bmm350/bmm350_wrapper.h"
#include "bmp581/bmp581_hal.h"
#include "lsm6dsv80x/lsm6dsv80x.h"

static uint32_t notified_value;
static stopwatch_t stopwatch;

void sensors_manager_init() {
    stopwatch_set_sensor_task_handle(xTaskGetCurrentTaskHandle());
    stopwatch_init();

    HAL_StatusTypeDef res = bmm350_init();
    if (res != HAL_OK) {
        LOG_ERROR("bmm350_init error");
    }
    res = lsm6dsv80x_init();
    if (res != HAL_OK) {
        LOG_ERROR("lsm6dsv80x_init error");
    }
    res = bmi088_init();
    if (res != HAL_OK) {
        LOG_ERROR("bmi08_init error");
    }
    res = bmp581_init();
    if (res != HAL_OK) {
        LOG_ERROR("bmp581_init error");
    }
}

static inline void sensors_manager_handle_bmm350() {
    double x, y, z;
    if (STOPWATCH_GET_BMM350(&stopwatch) == 0) {
        bmm350_get(&x, &y, &z);
        LOG_INFO("BMM350 x: %f y: %f z: %f, f: %f t: %ld\r\n", x, y, z, stopwatch.f_hz, stopwatch.t_us);
    }
}

static inline void sensors_manager_handle_lsm6dsv() {
    static lsm6dsv_data_t lsm6dsv;
    if (STOPWATCH_GET_LSM6DSV(&stopwatch) == 0) {
        lsm6dsv80x_get(&lsm6dsv);
        LOG_INFO("LSM6DSV_ACC   x: %f y: %f z: %f, f: %f t: %ld\r\n",
                 lsm6dsv.acc_x,
                 lsm6dsv.acc_y,
                 lsm6dsv.acc_z,
                 stopwatch.f_hz,
                 stopwatch.t_us);
        LOG_INFO("LSM6DSV_ACC_G x: %f y: %f z: %f, f: %f t: %ld\r\n",
                 lsm6dsv.acc_h_x,
                 lsm6dsv.acc_h_y,
                 lsm6dsv.acc_h_z,
                 stopwatch.f_hz,
                 stopwatch.t_us);
        LOG_INFO("LSM6DSV_GYR   x: %f y: %f z: %f, f: %f t: %ld\r\n",
                 lsm6dsv.gyr_x,
                 lsm6dsv.gyr_y,
                 lsm6dsv.gyr_z,
                 stopwatch.f_hz,
                 stopwatch.t_us);
    }
}

static inline void sensors_manager_handle_bmi088() {
    double x, y, z;
    if (STOPWATCH_GET_BMI088(&stopwatch) == 0) {
        bmi088_acc_get(&x, &y, &z);
        LOG_INFO("BMI088_ACC x: %f y: %f z: %f, f: %f t: %ld\r\n", x, y, z, stopwatch.f_hz, stopwatch.t_us);
        bmi088_gyr_get(&x, &y, &z);
        LOG_INFO("BMI088_GYR x: %f y: %f z: %f, f: %f t: %ld\r\n", x, y, z, stopwatch.f_hz, stopwatch.t_us);
    }
}

static inline void sensors_manager_handle_bmp581() {
    float pressure_pa, temperature_c;
    if (STOPWATCH_GET_BMP581(&stopwatch) == 0) {
        bmp581_get_data(&pressure_pa, &temperature_c);
        LOG_INFO(
          "BMP581_ACC pa: %f temp: %f f: %f t: %ld\r\n", pressure_pa, temperature_c, stopwatch.f_hz, stopwatch.t_us);
    }
}

static inline void sensors_manager_handle_data() {
    if (notified_value & (1 << SENSOR_BMM350)) {
        sensors_manager_handle_bmm350();
    }
    if (notified_value & (1 << SENSOR_LSM6DSV)) {
        sensors_manager_handle_lsm6dsv();
    }
    if (notified_value & (1 << SENSOR_BMI088)) {
        sensors_manager_handle_bmi088();
    }
    if (notified_value & (1 << SENSOR_BMP581)) {
        sensors_manager_handle_bmp581();
    }
}

void sensors_manager_process() {
    if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notified_value, portMAX_DELAY) == pdTRUE) {
        sensors_manager_handle_data();
    }
}