#include <assert.h>

#include <FreeRTOS.h>
#include <queue.h>

#include "queue_manager/queue_manager.h"

#define PITCH_QUEUE_ITEM_SIZE (sizeof(float))
#define PITCH_QUEUE_LEN       (1)

static StaticQueue_t pitchQueueBuffer;
static uint8_t pitchQueueStorageBuffer[PITCH_QUEUE_ITEM_SIZE * PITCH_QUEUE_LEN];

static QueueHandle_t queues[QUEUE_TYPE_SENTINEL];

void queue_manager_init() {
    queues[QUEUE_TYPE_PITCH]
      = xQueueCreateStatic(PITCH_QUEUE_LEN, PITCH_QUEUE_ITEM_SIZE, pitchQueueStorageBuffer, &pitchQueueBuffer);

    assert(queues[QUEUE_TYPE_PITCH] != NULL);
}

int32_t queue_manager_receive(const queue_type_t type, void *item, uint32_t timeout_ms) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    if (queues[type] == NULL) {
        return -1;
    }

    return (xQueueReceive(queues[type], item, pdMS_TO_TICKS(timeout_ms)) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_send(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    if (queues[type] == NULL) {
        return -1;
    }

    return (xQueueSend(queues[type], item, 0) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_send_isr(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    if (queues[type] == NULL) {
        return -1;
    }

    return (xQueueSendFromISR(queues[type], item, NULL) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_has_items(const queue_type_t type) {
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    if (queues[type] == NULL) {
        return 0;
    }

    return (int32_t)(uxQueueMessagesWaiting(queues[type]));
}

int32_t queue_manager_overwrite(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    if (queues[type] == NULL) {
        return -1;
    }

    return (xQueueOverwrite(queues[type], item) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_overwrite_isr(const queue_type_t type, void *item, BaseType_t *const pxHigherPriorityTaskWoken) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    if (queues[type] == NULL) {
        return -1;
    }

    return (xQueueOverwriteFromISR(queues[type], item, pxHigherPriorityTaskWoken) == pdFALSE) ? -1 : 0;
}

int32_t queue_manager_peek(const queue_type_t type, void *item) {
    assert(item != NULL);
    assert((uint8_t)type < QUEUE_TYPE_SENTINEL);

    if (queues[type] == NULL) {
        return -1;
    }

    return (xQueuePeek(queues[type], item, 0) == pdFALSE) ? -1 : 0;
}