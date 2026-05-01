#ifndef APP_DRIVERS_STEP_MANAGER_STEP_MANAGER_H_
#define APP_DRIVERS_STEP_MANAGER_STEP_MANAGER_H_

#include <stdint.h>

typedef enum { STEP_MOTOR_1 = 0, STEP_MOTOR_2, STEP_MOTOR_COUNT } step_motor_id_t;

void step_manager_init();
void step_manager_set_speed(step_motor_id_t motor_id, float rad_per_second);
void step_manager_step(step_motor_id_t motor_id);

float step_manager_get_speed(step_motor_id_t motor_id);
float step_manager_get_position(step_motor_id_t motor_id);
void step_manager_update_position(float dt);

#endif  // APP_DRIVERS_STEP_MANAGER_STEP_MANAGER_H_
