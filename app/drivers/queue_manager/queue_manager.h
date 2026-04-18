
#ifndef APP_DRIVERS_QUEUE_MANAGER_QUEUE_MANAGER_H_
#define APP_DRIVERS_QUEUE_MANAGER_QUEUE_MANAGER_H_

#include <FreeRTOS.h>
#include <inttypes.h>


typedef enum queue_type_e {
    QUEUE_TYPE_PITCH = 0,
    QUEUE_TYPE_SENTINEL,
} queue_type_t;

void queue_manager_init();
int32_t queue_manager_receive(const queue_type_t type, void *item, uint32_t timeout_ms);
int32_t queue_manager_send(const queue_type_t type, void *item);
int32_t queue_manager_send_isr(const queue_type_t type, void *item);
int32_t queue_manager_has_items(const queue_type_t type);
int32_t queue_manager_overwrite(const queue_type_t type, void *item);
int32_t queue_manager_overwrite_isr(const queue_type_t type, void *item, BaseType_t *const pxHigherPriorityTaskWoken);
int32_t queue_manager_peek(const queue_type_t type, void *item);

#define PITCH_QUEUE_PEEK(ITEM)              queue_manager_peek(QUEUE_TYPE_PITCH, ITEM)
#define PITCH_QUEUE_OVERRIDE_ISR(ITEM, HPT) queue_manager_overwrite_isr(QUEUE_TYPE_PITCH, ITEM, HPT)

#endif  // APP_DRIVERS_QUEUE_MANAGER_QUEUE_MANAGER_H_
