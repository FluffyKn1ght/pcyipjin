#include "utils/queue.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _check_ident(Queue* queue) {
    if (queue->ident != QUEUE_IDENT) {
        if (queue->ident == QUEUE_IDENT_FREED) {
            printf("utils/queue.c: passed freed queue pointer to queue function\n");
        } else {
            printf("utils/queue.c: passed invalid pointer to queue function\n");
        }

        assert(false);
    }
}

Queue* queue_create(uint32_t item_size) {
    if (item_size == 0) {
        return NULL;
    }

    Queue* queue = malloc(sizeof(Queue));

    queue->ident = QUEUE_IDENT;
    queue->item_size = item_size;
    queue->capacity = QUEUE_GROW_SIZE;
    queue->num_items = 0;
    queue->head_offset = 0;
    queue->tail_offset = 0;

    queue->data = malloc(queue->item_size * queue->capacity);

    return queue;
}

void queue_free(Queue* queue) {
    _check_ident(queue);

    free(queue->data);
    queue->ident = QUEUE_IDENT_FREED;
    free(queue);
}

void queue_enqueue(Queue* queue, void* src) {
    _check_ident(queue);

    if (queue->num_items >= queue->capacity) {
        queue_grow(queue, queue->capacity + QUEUE_GROW_SIZE);
    }

    queue->num_items++;
    memcpy(queue->data + queue->tail_offset, src, queue->item_size);
    queue->tail_offset += queue->item_size;
    queue->tail_offset %= queue->capacity * queue->item_size;
}

void* queue_peek(Queue* queue) {
    _check_ident(queue);

    if (!queue->num_items) {
        return NULL;
    }

    return (void*)(queue->head_offset - queue->item_size);
}

bool queue_dequeue(Queue* queue, void* dest) {
    _check_ident(queue);

    void* ptr = queue_peek(queue);

    if (!ptr) {
        return false;
    }

    queue->head_offset += queue->item_size;
    queue->head_offset %= queue->capacity * queue->item_size;

    if (dest) {
        memcpy(dest, ptr, queue->item_size);
    }

    return true;
}

bool queue_grow(Queue* queue, uint32_t target_capacity) {
    _check_ident(queue);

    if (queue->capacity >= target_capacity) {
        return false;
    }

    queue->capacity = target_capacity;

    void* data2 = malloc(queue->capacity * queue->item_size);
    memcpy(data2, queue->data + queue->head_offset,
           queue->num_items * queue->item_size - queue->head_offset);
    memcpy(data2 + (queue->num_items * queue->item_size - queue->head_offset),
           queue->data + queue->tail_offset, queue->tail_offset);

    queue->head_offset = 0;
    queue->tail_offset = queue->num_items * queue->item_size;

    free(queue->data);
    queue->data = data2;

    return true;
}

void queue_clear(Queue* queue) {
    _check_ident(queue);

    queue->head_offset = 0;
    queue->tail_offset = 0;
    queue->num_items = 0;
}
