#ifndef APP_DRIVERS_STEP_MANAGER_STEP_MANAGER_H_
#define APP_DRIVERS_STEP_MANAGER_STEP_MANAGER_H_

#include <stdint.h>

typedef enum { STEP_MOTOR_1 = 0, STEP_MOTOR_2, STEP_MOTOR_COUNT } step_motor_id_t;

void step_manager_init();
void step_manager_set_speed(step_motor_id_t motor_id, float steps_per_second);
void step_manager_step(step_motor_id_t motor_id);

#endif  // APP_DRIVERS_STEP_MANAGER_STEP_MANAGER_H_
