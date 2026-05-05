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

// Nasz wygenerowany plik z macierzami i definicjami (MPC_NX = 6, MPC_U_LEN = 40)
#include "mpc_data.h"

#define SENSORS_TASK_STACK_SIZE (1024)
#define MAX_WHEEL_SPEED_RADS    10.0f
#define MAX_FGM_ITER            30
#define U_MIN                   -100.0f
#define U_MAX                   100.0f

// Parametry zgodne z symulacją
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

// Bufory FGM
static float u_opt[MPC_U_LEN];
static float y_vec[MPC_U_LEN];
static float grad[MPC_U_LEN];

void solve_fgm_mpc(const float *x0, float *u_out) {
    // Warm-start
    for (int i = 0; i < MPC_U_LEN - MPC_NU; i++) {
        u_opt[i] = u_opt[i + MPC_NU];
        y_vec[i] = u_opt[i];
    }
    // Wyzerowanie ogona
    for (int i = MPC_U_LEN - MPC_NU; i < MPC_U_LEN; i++) {
        u_opt[i] = 0.0f;
        y_vec[i] = 0.0f;
    }

    float t = 1.0f;

    for (int iter = 0; iter < MAX_FGM_ITER; iter++) {
        // grad = H * y + F * x0
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

        // Gradient descent step + clipping
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

static void mpc_task(void *argument) {
    (void)argument;
    portTASK_USES_FLOATING_POINT();

    // --- KALIBRACJA OFFSETU PIONU ---
    LOG_INFO("MPC: Calibrating pitch offset (Stay still!)...\r\n");
    float pitch_offset       = 0.0f;
    int calibration_samples  = 0;
    const int target_samples = 20;

    while (calibration_samples < target_samples) {
        imu_data_t imu_raw;
        if (PITCH_QUEUE_PEEK(&imu_raw) == 0) {
            pitch_offset += imu_raw.pitch;
            calibration_samples++;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
    pitch_offset /= (float)target_samples;
    LOG_INFO("MPC: Calibration done. Offset: %.2f deg\r\n", pitch_offset);

    // Pozycja docelowa (0.0 = stój tam, gdzie zostałeś włączony)
    float target_x = 0.0f;

    HAL_TIM_Base_Start_IT(&htim3);

    while (1) {
        if (xSemaphoreTake(mpc_semaphore_handle, portMAX_DELAY) == pdPASS) {
            // Aktualizacja w timerach (zapisuje aktualną prędkość * dt do pozycji)
            step_manager_update_position(SYM_DT);

            imu_data_t imu_data;
            if (PITCH_QUEUE_PEEK(&imu_data) == 0) {
                // 1. Kąt z IMU w radianach
                float phi  = (imu_data.pitch - pitch_offset) * (M_PI / 180.0f);
                float phip = imu_data.pitch_dot * (M_PI / 180.0f);

                // 2. Pobieranie gotowych danych ze step_managera
                float actual_omega_L = -step_manager_get_speed(STEP_MOTOR_1);
                float actual_omega_R = -step_manager_get_speed(STEP_MOTOR_2);

                float actual_pos_L = -step_manager_get_position(STEP_MOTOR_1);
                float actual_pos_R = -step_manager_get_position(STEP_MOTOR_2);

                // ========================================================
                // 3. OBLICZENIA DLA MPC (Zmienne stanu wg modelu SymPy)
                // ========================================================
                float current_vel_L_mpc = actual_omega_L * WHEEL_RADIUS;
                float current_vel_R_mpc = actual_omega_R * WHEEL_RADIUS;

                // Prędkość liniowa kół
                float xp = (current_vel_L_mpc + current_vel_R_mpc) / 2.0f;

                // Pozycja liniowa z kół (średni przejechany dystans) + wpływ pochylenia robota na środek ciężkości
                float x_wheels = ((actual_pos_L + actual_pos_R) / 2.0f) * WHEEL_RADIUS;

                // 4. Budowa Wektora Stanu
                float x0[MPC_NX];
                x0[0] = x_wheels;  // Błąd pozycji X
                x0[1] = xp;        // Prędkość
                x0[2] = phi;       // Pochylenie
                x0[3] = phip;      // Prędkość pochylania
                x0[4] = 0.0f;      // Ignorujemy kąt skrętu (Psi)
                x0[5] = 0.0f;      // Ignorujemy prędkość skręcania (Psip)

                // 5. Rozwiązanie MPC
                float u0[MPC_NU];
                solve_fgm_mpc(x0, u0);

                float a_L = u0[0];
                float a_R = u0[1];

                // 6. Całkowanie do docelowych prędkości wg modelu SymPy
                float v_L_target = current_vel_L_mpc + a_L * SYM_DT;
                float v_R_target = current_vel_R_mpc + a_R * SYM_DT;

                // 7. Tłumaczenie na rad/s
                float omega_L = v_L_target / WHEEL_RADIUS;
                float omega_R = v_R_target / WHEEL_RADIUS;

                // 8. Clipping i zadanie prędkości
                if (omega_L > MAX_WHEEL_SPEED_RADS)
                    omega_L = MAX_WHEEL_SPEED_RADS;
                if (omega_L < -MAX_WHEEL_SPEED_RADS)
                    omega_L = -MAX_WHEEL_SPEED_RADS;
                if (omega_R > MAX_WHEEL_SPEED_RADS)
                    omega_R = MAX_WHEEL_SPEED_RADS;
                if (omega_R < -MAX_WHEEL_SPEED_RADS)
                    omega_R = -MAX_WHEEL_SPEED_RADS;

                // LOG_INFO("X: %.3f | X_err: %.3f | Phi: %.2f\n\r", global_pos_x, x0[0], imu_data.pitch);

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