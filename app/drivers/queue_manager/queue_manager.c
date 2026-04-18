#include <assert.h>

#include <FreeRTOS.h>
#include <queue.h>

#include "queue_manager/queue_manager.h"

#define EXAMPLE_QUEUE_ITEM_SIZE (sizeof(int))
#define EXAMPLE_QUEUE_LEN       (10)

static StaticQueue_t exampleQueueBuffer;
static uint8_t exampleQueueStorageBuffer[EXAMPLE_QUEUE_ITEM_SIZE * EXAMPLE_QUEUE_LEN];

static QueueHandle_t queues[QUEUE_TYPE_SENTINEL];

void queue_manager_init() {
    queues[QUEUE_TYPE_EXAMPLE]
      = xQueueCreateStatic(EXAMPLE_QUEUE_LEN, EXAMPLE_QUEUE_ITEM_SIZE, exampleQueueStorageBuffer, &exampleQueueBuffer);

    assert(queues[QUEUE_TYPE_EXAMPLE] != NULL);
}

int32_t queue_manager_receive(const queue_type_t type, void *item, uint32_t timeout_ms) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    return (xQueueReceive(queues[type], item, pdMS_TO_TICKS(timeout_ms)) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_send(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    return (xQueueSend(queues[type], item, 0) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_send_isr(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    return (xQueueSendFromISR(queues[type], item, 0) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_has_items(const queue_type_t type) {
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);
    return (int32_t)(uxQueueMessagesWaiting(queues[type]));
}

int32_t queue_manager_overwrite(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    return (xQueueOverwrite(queues[type], item) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_peek(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    return (xQueuePeek(queues[type], item, 0) == pdFALSE) ? -1 : 0;
}