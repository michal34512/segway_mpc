
#ifndef APP_DRIVERS_QUEUE_MANAGER_QUEUE_MANAGER_H_
#define APP_DRIVERS_QUEUE_MANAGER_QUEUE_MANAGER_H_

#include <inttypes.h>

typedef enum queue_type_e {
    QUEUE_TYPE_EXAMPLE = 0,
    QUEUE_TYPE_SENTINEL,
} queue_type_t;

void queue_manager_init();
int32_t queue_manager_receive(const queue_type_t type, void *item, uint32_t timeout_ms);
int32_t queue_manager_send(const queue_type_t type, void *item);
int32_t queue_manager_send_isr(const queue_type_t type, void *item);
int32_t queue_manager_has_items(const queue_type_t type);
int32_t queue_manager_overwrite(const queue_type_t type, void *item);
int32_t queue_manager_peek(const queue_type_t type, void *item);

#define EXAMPLE_QUEUE_RECEIVE(ITEM, TIMEOUT) queue_manager_receive(QUEUE_TYPE_EXAMPLE, ITEM, TIMEOUT)
#define EXAMPLE_QUEUE_SEND(ITEM)             queue_manager_send(QUEUE_TYPE_EXAMPLE, ITEM)
#define EXAMPLE_QUEUE_SEND_ISR(ITEM)         queue_manager_send_isr(QUEUE_TYPE_EXAMPLE, ITEM)

#endif  // APP_DRIVERS_QUEUE_MANAGER_QUEUE_MANAGER_H_
