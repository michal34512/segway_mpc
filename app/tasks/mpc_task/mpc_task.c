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

#define SUCCESS HPIPM_SUCCESS
#include "acados_solver_segway_linear_mpc.h"
#undef SUCCESS

#define SENSORS_TASK_STACK_SIZE (1024)
#define MAX_WHEEL_SPEED_RADS    10.0f

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

// ZMIANA: Zmiana nazwy struktury pamięci na liniową
static segway_linear_mpc_solver_capsule mpc_capsule_memory;

void mpc_wake_up() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(mpc_semaphore_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void mpc_task(void *argument) {
    (void)argument;
    portTASK_USES_FLOATING_POINT();

    // ZMIANA: Użycie nowego, lekkiego solwera LMPC
    segway_linear_mpc_solver_capsule *mpc_capsule = &mpc_capsule_memory;
    int status                                    = segway_linear_mpc_acados_create(mpc_capsule);

    if (status != 0) {
        LOG_ERROR("Acados create failed: %d\r\n", status);
        while (1) {
            vTaskDelay(1000);
        }
    }

    // --- KALIBRACJA OFFSETU PIONU ---
    LOG_INFO("MPC: Calibrating pitch offset (Stay still!)...\r\n");
    float pitch_offset       = 0.0f;
    int calibration_samples  = 0;
    const int target_samples = 20;  // Zbieramy 20 próbek (ok. 0.4s przy 50Hz)

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

    // ZMIANA: nx = 4, nu = 2
    double x0[SEGWAY_LINEAR_MPC_NX] = {0.0};
    double u0[SEGWAY_LINEAR_MPC_NU] = {0.0};

    // Usunięto niepotrzebną zmienną current_pos_x - nie całkujemy już pozycji absolutnej dla MPC

    // Warm-start horyzontu
    for (int i = 0; i <= mpc_capsule->nlp_dims->N; i++) {
        ocp_nlp_out_set(
          mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_out, mpc_capsule->nlp_in, i, "x", x0);
        ocp_nlp_out_set(
          mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_out, mpc_capsule->nlp_in, i, "u", u0);
    }

    HAL_TIM_Base_Start_IT(&htim3);

    while (1) {
        if (xSemaphoreTake(mpc_semaphore_handle, portMAX_DELAY) == pdPASS) {
            step_manager_update_position(SYM_DT);
            imu_data_t imu_data;
            if (PITCH_QUEUE_PEEK(&imu_data) == 0) {
                // Aplikacja offsetu i konwersja na radiany
                float phi  = (imu_data.pitch - pitch_offset) * (M_PI / 180.0f);
                float phip = (imu_data.pitch_dot) * (M_PI / 180.0f);

                float actual_omega_L = -step_manager_get_speed(STEP_MOTOR_1);
                float actual_omega_R = -step_manager_get_speed(STEP_MOTOR_2);

                float current_vel_L = (actual_omega_L + phip) * WHEEL_RADIUS;
                float current_vel_R = (actual_omega_R + phip) * WHEEL_RADIUS;
                // Obliczamy prędkość liniową środka robota (xp) oraz prędkość obrotu (psip)

                float xp   = (current_vel_L + current_vel_R) / 2.0f;
                float psip = (current_vel_R - current_vel_L) / WHEEL_SPACING;

                // ZMIANA: Nowy 4-elementowy wektor stanu: state = [xp, phi, phip, psip]
                x0[0] = xp;    // xp (Linear velocity)
                x0[1] = phi;   // phi (Pitch - teraz na indeksie 1!)
                x0[2] = phip;  // phip (Pitch rate)
                x0[3] = psip;  // psip (Yaw rate)

                ocp_nlp_constraints_model_set(mpc_capsule->nlp_config,
                                              mpc_capsule->nlp_dims,
                                              mpc_capsule->nlp_in,
                                              mpc_capsule->nlp_out,
                                              0,
                                              "lbx",
                                              x0);
                ocp_nlp_constraints_model_set(mpc_capsule->nlp_config,
                                              mpc_capsule->nlp_dims,
                                              mpc_capsule->nlp_in,
                                              mpc_capsule->nlp_out,
                                              0,
                                              "ubx",
                                              x0);

                // ZMIANA: Wywołanie solve dla liniowego modelu
                status = segway_linear_mpc_acados_solve(mpc_capsule);

                // ZMIANA: qpOASES może czasem zwrócić 2 (max iter reached), co jest nadal użytecznym wynikiem
                if (status == 0 || status == 2) {
                    ocp_nlp_out_get(mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_out, 0, "u", u0);

                    // MPC wypluwa przyspieszenia a_L i a_R [m/s^2]
                    float a_L = (float)u0[0];
                    float a_R = (float)u0[1];

                    // 2. Całkujemy do prędkości liniowych kół [m/s]
                    float v_L_target = current_vel_L + a_L * SYM_DT;
                    float v_R_target = current_vel_R + a_R * SYM_DT;

                    // 3. Poprawka kinematyczna (omega = v/R - phip)
                    float omega_L = (v_L_target / WHEEL_RADIUS) - phip;
                    float omega_R = (v_R_target / WHEEL_RADIUS) - phip;

                    // Ograniczenie prędkości kół z zabezpieczeniem anti-windup
                    if (omega_L > MAX_WHEEL_SPEED_RADS)
                        omega_L = MAX_WHEEL_SPEED_RADS;
                    else if (omega_L < -MAX_WHEEL_SPEED_RADS)
                        omega_L = -MAX_WHEEL_SPEED_RADS;

                    if (omega_R > MAX_WHEEL_SPEED_RADS)
                        omega_R = MAX_WHEEL_SPEED_RADS;
                    else if (omega_R < -MAX_WHEEL_SPEED_RADS)
                        omega_R = -MAX_WHEEL_SPEED_RADS;

                    // Przeliczenie z powrotem na prędkość liniową wózka, aby zapobiec odkładaniu błędu
                    v_L_target = (omega_L + phip) * WHEEL_RADIUS;
                    v_R_target = (omega_R + phip) * WHEEL_RADIUS;

                    // LOG_INFO("Setting speed %f, %f\n\r", -omega_L, -omega_R);
                    step_manager_set_speed(STEP_MOTOR_1, -omega_L);
                    step_manager_set_speed(STEP_MOTOR_2, -omega_R);

                } else {
                    LOG_ERROR("MPC Error: %d\r\n", status);  // Opcjonalny log żeby wiedzieć, jak padnie całkowicie
                    step_manager_set_speed(STEP_MOTOR_1, 0);
                    step_manager_set_speed(STEP_MOTOR_2, 0);
                }
            } else {
                LOG_ERROR("Pitch queue Error: %d\r\n", status);
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