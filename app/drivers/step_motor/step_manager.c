#include "step_manager.h"
#include "main.h"
#include "tim.h"
#include <math.h>

#define STEP_PULSE_WIDTH_US 10
#define TIMER_CLOCK_FREQ    275000000
#define TIMER_PRESCALER     ((TIMER_CLOCK_FREQ / 1000000) - 1)
#define MOTOR_STEPS_PER_REV 200.0f

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    GPIO_TypeDef *dir_port;
    uint16_t dir_pin;
    uint8_t dir_inverted;
    GPIO_TypeDef *ms1_port;
    uint16_t ms1_pin;
    GPIO_TypeDef *ms2_port;
    uint16_t ms2_pin;
    GPIO_TypeDef *ms3_port;
    uint16_t ms3_pin;

    float current_speed_rad_s;
    float current_position_rad;
} motor_config_t;

static motor_config_t motors[STEP_MOTOR_COUNT] = {
  [STEP_MOTOR_1] = {.htim         = &htim2,
                    .channel      = TIM_CHANNEL_4,
                    .dir_port     = MOT1_DIR_GPIO_Port,
                    .dir_pin      = MOT1_DIR_Pin,
                    .dir_inverted = 0,
                    .ms1_port     = MOT1_MS1_GPIO_Port,
                    .ms1_pin      = MOT1_MS1_Pin,
                    .ms2_port     = MOT1_MS2_GPIO_Port,
                    .ms2_pin      = MOT1_MS2_Pin,
                    .ms3_port     = MOT1_MS3_GPIO_Port,
                    .ms3_pin      = MOT1_MS3_Pin},
  [STEP_MOTOR_2] = {.htim         = &htim4,
                    .channel      = TIM_CHANNEL_3,
                    .dir_port     = MOT2_DIR_GPIO_Port,
                    .dir_pin      = MOT2_DIR_Pin,
                    .dir_inverted = 1,
                    .ms1_port     = MOT2_MS1_GPIO_Port,
                    .ms1_pin      = MOT2_MS1_Pin,
                    .ms2_port     = MOT2_MS2_GPIO_Port,
                    .ms2_pin      = MOT2_MS2_Pin,
                    .ms3_port     = MOT2_MS3_GPIO_Port,
                    .ms3_pin      = MOT2_MS3_Pin},
};

void step_manager_init() {
    for (int i = 0; i < STEP_MOTOR_COUNT; i++) {
        TIM_HandleTypeDef *htim      = motors[i].htim;
        uint32_t channel             = motors[i].channel;
        TIM_OC_InitTypeDef sConfigOC = {0};
        htim->Instance->CR1 &= ~TIM_CR1_OPM;
        htim->Instance->CR1 &= ~TIM_CR1_ARPE;

        sConfigOC.OCMode       = TIM_OCMODE_PWM1;
        sConfigOC.Pulse        = STEP_PULSE_WIDTH_US;
        sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
        sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
        sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
        sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;

        if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel) != HAL_OK) {
            Error_Handler();
        }

        __HAL_TIM_SET_PRESCALER(htim, TIMER_PRESCALER);
        HAL_TIM_PWM_Stop(htim, channel);
    }
}

void step_manager_set_speed(step_motor_id_t motor_id, float rad_per_second) {
    if (motor_id >= STEP_MOTOR_COUNT) {
        return;
    }

    TIM_HandleTypeDef *htim = motors[motor_id].htim;
    uint32_t channel        = motors[motor_id].channel;

    // 1. Kierunek
    uint8_t is_negative = (rad_per_second < 0.0f) ? 1 : 0;
    uint8_t pin_state   = is_negative ^ motors[motor_id].dir_inverted;
    HAL_GPIO_WritePin(motors[motor_id].dir_port, motors[motor_id].dir_pin, pin_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    float abs_rad_speed   = fabsf(rad_per_second);
    float abs_steps_speed = abs_rad_speed * (MOTOR_STEPS_PER_REV / (2.0f * (float)M_PI));

    // FIX 1: SZTYWNY MICROSTEPPING.
    // Brak szarpania fazy sterownika! Ustawiamy np. na 1/16 kroku.
    uint8_t microstep_mode = 16;
    HAL_GPIO_WritePin(motors[motor_id].ms1_port, motors[motor_id].ms1_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motors[motor_id].ms2_port, motors[motor_id].ms2_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motors[motor_id].ms3_port, motors[motor_id].ms3_pin, GPIO_PIN_SET);

    float hardware_speed       = abs_steps_speed * (float)microstep_mode;
    uint32_t max_period        = 0xFFFF;
    float min_steps_per_second = (float)(TIMER_CLOCK_FREQ / (TIMER_PRESCALER + 1)) / (float)max_period;

    if (hardware_speed <= min_steps_per_second) {
        // FIX 2: Zatrzymujemy tylko, jeśli silnik faktycznie się kręcił
        if (motors[motor_id].current_speed_rad_s != 0.0f) {
            HAL_TIM_PWM_Stop(htim, channel);
            motors[motor_id].current_speed_rad_s = 0.0f;
        }
    } else {
        uint32_t period = (uint32_t)((float)(TIMER_CLOCK_FREQ / (TIMER_PRESCALER + 1)) / hardware_speed);
        if (period <= STEP_PULSE_WIDTH_US) {
            period = STEP_PULSE_WIDTH_US + 1;
        }

        // Ustawiamy nowy okres
        __HAL_TIM_SET_AUTORELOAD(htim, period - 1);

        // Agresywny reset licznika, jeśli nowy okres jest krótszy niż obecny stan
        if (__HAL_TIM_GET_COUNTER(htim) >= period) {
            htim->Instance->EGR = TIM_EGR_UG;
            htim->Instance->SR  = ~TIM_SR_UIF;  // Czyścimy flagę, by nie było lewego przerwania
        }

        // FIX 3: Startujemy timer tylko wtedy, gdy ruszamy z postoju
        if (motors[motor_id].current_speed_rad_s == 0.0f) {
            __HAL_TIM_SET_COUNTER(htim, 0);  // Zerujemy licznik na start
            HAL_TIM_PWM_Start(htim, channel);
        }

        motors[motor_id].current_speed_rad_s = rad_per_second;
    }
}

void step_manager_step(step_motor_id_t motor_id) {
    if (motor_id >= STEP_MOTOR_COUNT) {
        return;
    }
    HAL_TIM_GenerateEvent(motors[motor_id].htim, TIM_EVENTSOURCE_UPDATE);
}

float step_manager_get_speed(step_motor_id_t motor_id) {
    if (motor_id >= STEP_MOTOR_COUNT) {
        return 0.0f;
    }
    return motors[motor_id].current_speed_rad_s;
}

float step_manager_get_position(step_motor_id_t motor_id) {
    if (motor_id >= STEP_MOTOR_COUNT) {
        return 0.0f;
    }
    return motors[motor_id].current_position_rad;
}

void step_manager_update_position(float dt) {
    for (int i = 0; i < STEP_MOTOR_COUNT; i++) {
        motors[i].current_position_rad += motors[i].current_speed_rad_s * dt;
    }
}