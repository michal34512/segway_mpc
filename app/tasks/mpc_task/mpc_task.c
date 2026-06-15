#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <math.h>
#include <semphr.h>
#include <stdint.h>
#include <stm32h7xx_hal.h>
#include <task.h>
#include <tim.h>

#include "logger/logger.h"
#include "mpc_task/mpc_task.h"
#include "queue_manager/queue_manager.h"
#include "step_motor/step_manager.h"

#include "mpc_data.h"

#define SENSORS_TASK_STACK_SIZE (1024)
#define MAX_WHEEL_SPEED_RADS    10.0f
#define MAX_FGM_ITER            30
#define U_MIN                   -100.0f
#define U_MAX                   100.0f

#define SYM_DT        0.02f
#define WHEEL_RADIUS  0.04f
#define WHEEL_SPACING 0.2f

static osThreadId_t mpc_task_handle;
static uint32_t mpc_task_buffer[SENSORS_TASK_STACK_SIZE];
static StaticTask_t mpc_task_control_block;

static const osThreadAttr_t mpc_task_attributes = {
  .name       = "MPC Task",
  .stack_mem  = &mpc_task_buffer,
  .stack_size = sizeof(mpc_task_buffer),
  .cb_mem     = &mpc_task_control_block,
  .cb_size    = sizeof(mpc_task_control_block),
  .priority   = (osPriority_t)osPriorityNormal,
};

static SemaphoreHandle_t mpc_semaphore_handle;

static float u_opt[MPC_U_LEN];
static float y_vec[MPC_U_LEN];
static float grad[MPC_U_LEN];

void solve_fgm_mpc(const float *x0, float *u_out) {
    for (int i = 0; i < MPC_U_LEN - MPC_NU; i++) {
        u_opt[i] = u_opt[i + MPC_NU];
        y_vec[i] = u_opt[i];
    }

    for (int i = MPC_U_LEN - MPC_NU; i < MPC_U_LEN; i++) {
        u_opt[i] = 0.0f;
        y_vec[i] = 0.0f;
    }

    float t = 1.0f;

    for (int iter = 0; iter < MAX_FGM_ITER; iter++) {
        for (int i = 0; i < MPC_U_LEN; i++) {
            grad[i] = 0.0f;
            for (int j = 0; j < MPC_U_LEN; j++) {
                grad[i] += MPC_H[i * MPC_U_LEN + j] * y_vec[j];
            }
            for (int k = 0; k < MPC_NX; k++) {
                grad[i] += MPC_F[i * MPC_NX + k] * x0[k];
            }
        }

        float t_next = (1.0f + sqrtf(1.0f + 4.0f * t * t)) / 2.0f;
        float beta   = (t - 1.0f) / t_next;
        t            = t_next;

        for (int i = 0; i < MPC_U_LEN; i++) {
            float u_old = u_opt[i];
            float u_new = y_vec[i] - MPC_L_INV * grad[i];

            if (u_new > U_MAX)
                u_new = U_MAX;
            if (u_new < U_MIN)
                u_new = U_MIN;

            u_opt[i] = u_new;
            y_vec[i] = u_opt[i] + beta * (u_opt[i] - u_old);
        }
    }

    u_out[0] = u_opt[0];
    u_out[1] = u_opt[1];
}

void mpc_wake_up() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(mpc_semaphore_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

typedef struct {
    float pos;
    float vel;
} trajectory_point_t;

static trajectory_point_t trajectory_generator(float t) {
    trajectory_point_t ref;

    if (t < 2.0f) {
        ref.pos = 0.5f * t;
        ref.vel = 0.5f;
    } else if (t < 4.0f) {
        ref.pos = 1.0f;
        ref.vel = 0.0f;
    } else if (t < 6.0f) {
        ref.pos = 1.0f - 0.5f * (t - 4.0f);
        ref.vel = -0.5f;
    } else {
        ref.pos = 0.0f;
        ref.vel = 0.0f;
    }

    return ref;
}

static float figure8_turn_generator(float t) {
    const float T = 8.0f;
    return 1.8f * sinf(4.0f * M_PI * t / T);
}

static void mpc_task(void *argument) {
    (void)argument;
    portTASK_USES_FLOATING_POINT();

    float pitch_offset      = 0.0f;
    int calibration_samples = 0;

    while (calibration_samples < 20) {
        imu_data_t imu_raw;
        if (PITCH_QUEUE_PEEK(&imu_raw) == 0) {
            pitch_offset += imu_raw.pitch;
            calibration_samples++;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    pitch_offset /= 20.0f;

    float traj_time = 0.0f;

    HAL_TIM_Base_Start_IT(&htim3);

    while (1) {
        if (xSemaphoreTake(mpc_semaphore_handle, portMAX_DELAY) == pdPASS) {
            step_manager_update_position(SYM_DT);
            traj_time += SYM_DT;

            trajectory_point_t ref = trajectory_generator(traj_time);

            imu_data_t imu_data;

            if (PITCH_QUEUE_PEEK(&imu_data) == 0) {
                float phi  = (imu_data.pitch - pitch_offset) * (M_PI / 180.0f);
                float phip = imu_data.pitch_dot * (M_PI / 180.0f);

                float actual_omega_L = -step_manager_get_speed(STEP_MOTOR_1);
                float actual_omega_R = -step_manager_get_speed(STEP_MOTOR_2);

                float actual_pos_L = -step_manager_get_position(STEP_MOTOR_1);
                float actual_pos_R = -step_manager_get_position(STEP_MOTOR_2);

                float current_vel_L_mpc = actual_omega_L * WHEEL_RADIUS;
                float current_vel_R_mpc = actual_omega_R * WHEEL_RADIUS;

                float xp = (current_vel_L_mpc + current_vel_R_mpc) / 2.0f;

                float x_wheels = ((actual_pos_L + actual_pos_R) / 2.0f) * WHEEL_RADIUS;

                float x0[MPC_NX];

                x0[0] = x_wheels - ref.pos;
                x0[1] = xp - ref.vel;
                x0[2] = phi;
                x0[3] = phip;
                x0[4] = 0.0f;
                x0[5] = 0.0f;

                float u0[MPC_NU];
                solve_fgm_mpc(x0, u0);

                float a_L = u0[0];
                float a_R = u0[1];

                float v_L_target = current_vel_L_mpc + a_L * SYM_DT;
                float v_R_target = current_vel_R_mpc + a_R * SYM_DT;

                float omega_forward_L = v_L_target / WHEEL_RADIUS;
                float omega_forward_R = v_R_target / WHEEL_RADIUS;

                float turn = figure8_turn_generator(traj_time);

                float omega_L = omega_forward_L - turn;
                float omega_R = omega_forward_R + turn;

                if (omega_L > MAX_WHEEL_SPEED_RADS)
                    omega_L = MAX_WHEEL_SPEED_RADS;
                if (omega_L < -MAX_WHEEL_SPEED_RADS)
                    omega_L = -MAX_WHEEL_SPEED_RADS;

                if (omega_R > MAX_WHEEL_SPEED_RADS)
                    omega_R = MAX_WHEEL_SPEED_RADS;
                if (omega_R < -MAX_WHEEL_SPEED_RADS)
                    omega_R = -MAX_WHEEL_SPEED_RADS;

                LOG_INFO("ref=%.3f x=%.3f err=%.3f v=%.3f phi=%.2f\r\n", ref.pos, x_wheels, x0[0], xp, imu_data.pitch);
                step_manager_set_speed(STEP_MOTOR_1, -omega_L);
                step_manager_set_speed(STEP_MOTOR_2, -omega_R);
            } else {
                LOG_ERROR("IMU Data missing\r\n");
                step_manager_set_speed(STEP_MOTOR_1, 0);
                step_manager_set_speed(STEP_MOTOR_2, 0);
            }
        }
    }
}

void mpc_task_init() {
    mpc_semaphore_handle = xSemaphoreCreateBinary();
    mpc_task_handle      = osThreadNew(mpc_task, NULL, &mpc_task_attributes);

    assert_param(mpc_semaphore_handle);
    assert_param(mpc_task_handle);
}