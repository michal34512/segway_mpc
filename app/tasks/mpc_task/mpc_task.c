#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <math.h>
#include <stdint.h>
#include <stm32h7xx_hal.h>
#include <task.h>

#include "logger/logger.h"
#include "mpc_task/mpc_task.h"
#include "queue_manager/queue_manager.h"
#include "step_motor/step_manager.h"

// --- ACADOS INCLUDES ---
#define SUCCESS HPIPM_SUCCESS
#include "acados_solver_segway_nonlinear_mpc.h"
#undef SUCCESS

#define SENSORS_TASK_STACK_SIZE (1024)
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

static segway_nonlinear_mpc_solver_capsule mpc_capsule_memory;

static void mpc_task(void *argument) {
    (void)argument;
    portTASK_USES_FLOATING_POINT();

    segway_nonlinear_mpc_solver_capsule *mpc_capsule = &mpc_capsule_memory;
    int status                                       = segway_nonlinear_mpc_acados_create(mpc_capsule);

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

    // Parametry zgodne z symulacją
    const float dt      = 0.02f;    // 50 Hz (FPS=50 w Pythonie)
    const float R_wheel = 0.0325f;  // Promień koła w metrach (dostosuj!)
    const float L_width = 0.15f;    // Rozstaw kół (dostosuj!)

    double x0[SEGWAY_NONLINEAR_MPC_NX] = {0.0};
    double u0[SEGWAY_NONLINEAR_MPC_NU] = {0.0};

    float current_pos_x   = 0.0f;
    float current_vel_L   = 0.0f;  // Prędkość liniowa lewego koła [m/s]
    float current_vel_R   = 0.0f;  // Prędkość liniowa prawego koła [m/s]
    float current_phi_dot = 0.0f;
    float current_psi_dot = 0.0f;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Warm-start horyzontu
    for (int i = 0; i <= mpc_capsule->nlp_dims->N; i++) {
        ocp_nlp_out_set(
          mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_out, mpc_capsule->nlp_in, i, "x", x0);
        ocp_nlp_out_set(
          mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_out, mpc_capsule->nlp_in, i, "u", u0);
    }

    while (1) {
        imu_data_t imu_data;
        if (PITCH_QUEUE_PEEK(&imu_data) == 0) {
            // Aplikacja offsetu i konwersja na radiany
            float phi  = (imu_data.pitch - pitch_offset) * (M_PI / 180.0f);
            float phip = (imu_data.pitch_dot / 16.4f) * (M_PI / 180.0f);

            // Obliczamy prędkość liniową środka robota (xp) oraz prędkość obrotu (psip)
            float xp   = (current_vel_L + current_vel_R) / 2.0f;
            float psip = (current_vel_R - current_vel_L) / L_width;

            // Integracja pozycji (xp * dt)
            current_pos_x += xp * dt;

            // 1. Stan zgodny z symulacją (state = [0.0, xp, phi, phip, 0.0, psip])
            x0[0] = 0.0;   // X position (w Twojej symulacji zresetowane do 0 dla błędu)
            x0[1] = xp;    // xp (Linear velocity)
            x0[2] = phi;   // phi (Pitch)
            x0[3] = phip;  // phip (Pitch rate)
            x0[4] = 0.0;   // psi (Yaw - resetowane do 0)
            x0[5] = 0;     // psip (Yaw rate)

            ocp_nlp_constraints_model_set(
              mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_in, mpc_capsule->nlp_out, 0, "lbx", x0);
            ocp_nlp_constraints_model_set(
              mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_in, mpc_capsule->nlp_out, 0, "ubx", x0);

            status = segway_nonlinear_mpc_acados_solve(mpc_capsule);

            if (status == 0) {
                ocp_nlp_out_get(mpc_capsule->nlp_config, mpc_capsule->nlp_dims, mpc_capsule->nlp_out, 0, "u", u0);

                // MPC wypluwa przyspieszenia a_L i a_R [m/s^2]
                float a_L = (float)u0[0];
                float a_R = (float)u0[1];

                // 2. Całkujemy do prędkości liniowych kół [m/s]
                // v_target = v_current + a * dt
                float v_L_target = current_vel_L + a_L * dt;
                float v_R_target = current_vel_R + a_R * dt;

                // 3. Poprawka kinematyczna (omega = v/R - phip)
                float omega_L = (v_L_target / R_wheel) - phip;
                float omega_R = (v_R_target / R_wheel) - phip;

                // Update stanów dla następnej iteracji
                current_vel_L = v_L_target;
                current_vel_R = v_R_target;

                // Przeliczenie na kroki na sekundę
                float steps_L = omega_L * (20.0f / M_PI);
                float steps_R = omega_R * (20.0f / M_PI);

                step_manager_set_speed(STEP_MOTOR_1, -steps_L);
                step_manager_set_speed(STEP_MOTOR_2, -steps_R);

            } else {
                current_vel_L = 0;
                current_vel_R = 0;
                step_manager_set_speed(STEP_MOTOR_1, 0);
                step_manager_set_speed(STEP_MOTOR_2, 0);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));  // 50Hz
    }
}

void mpc_task_init() {
    mpc_task_handle = osThreadNew(mpc_task, NULL, &mpc_task_attributes);
    assert_param(mpc_task_handle);
}
