#include "step_manager.h"
#include "main.h"
#include "tim.h"
#include <math.h>

#define STEP_PULSE_WIDTH_US 10
#define TIMER_CLOCK_FREQ    550000000
#define TIMER_PRESCALER     ((TIMER_CLOCK_FREQ / 1000000) - 1)

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    GPIO_TypeDef *dir_port;
    uint16_t dir_pin;
    uint8_t dir_inverted;
} motor_config_t;

static motor_config_t motors[STEP_MOTOR_COUNT] = {
  [STEP_MOTOR_1] = {.htim         = &htim2,
                    .channel      = TIM_CHANNEL_4,
                    .dir_port     = MOT1_DIR_GPIO_Port,
                    .dir_pin      = MOT1_DIR_Pin,
                    .dir_inverted = 1},
  [STEP_MOTOR_2] = {.htim         = &htim4,
                    .channel      = TIM_CHANNEL_3,
                    .dir_port     = MOT2_DIR_GPIO_Port,
                    .dir_pin      = MOT2_DIR_Pin,
                    .dir_inverted = 0},
};

void step_manager_init() {
    for (int i = 0; i < STEP_MOTOR_COUNT; i++) {
        TIM_HandleTypeDef *htim      = motors[i].htim;
        uint32_t channel             = motors[i].channel;
        TIM_OC_InitTypeDef sConfigOC = {0};
        htim->Instance->CR1 &= ~TIM_CR1_OPM;

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

void step_manager_set_speed(step_motor_id_t motor_id, float steps_per_second) {
    if (motor_id >= STEP_MOTOR_COUNT) {
        return;
    }

    TIM_HandleTypeDef *htim = motors[motor_id].htim;
    uint32_t channel        = motors[motor_id].channel;
    uint8_t is_negative     = (steps_per_second < 0.0f) ? 1 : 0;
    uint8_t pin_state       = is_negative ^ motors[motor_id].dir_inverted;
    HAL_GPIO_WritePin(motors[motor_id].dir_port, motors[motor_id].dir_pin, pin_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    float abs_speed            = fabsf(steps_per_second);
    uint32_t max_period        = 0xFFFF;
    float min_steps_per_second = (float)(TIMER_CLOCK_FREQ / (TIMER_PRESCALER + 1)) / (float)max_period;
    if (abs_speed <= min_steps_per_second) {
        HAL_TIM_PWM_Stop(htim, channel);
    } else {
        uint32_t period = (uint32_t)((float)(TIMER_CLOCK_FREQ / (TIMER_PRESCALER + 1)) / abs_speed);
        if (period <= STEP_PULSE_WIDTH_US)
            period = STEP_PULSE_WIDTH_US + 1;

        __HAL_TIM_SET_AUTORELOAD(htim, period - 1);
        if (__HAL_TIM_GET_COUNTER(htim) >= period) {
            __HAL_TIM_SET_COUNTER(htim, 0);
        }
        HAL_TIM_PWM_Start(htim, channel);
    }
}

void step_manager_step(step_motor_id_t motor_id) {
    if (motor_id >= STEP_MOTOR_COUNT) {
        return;
    }
    HAL_TIM_GenerateEvent(motors[motor_id].htim, TIM_EVENTSOURCE_UPDATE);
}