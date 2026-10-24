/**
 * @file utils/queue.h
 * @brief Queue (FIFO) buffer implementation
 * @author FluffyKn1ght
 */

// shoutout to fgetcat for the head/tail strategy

#ifndef _FLUFFY_QUEUE_H
#define _FLUFFY_QUEUE_H

#define QUEUE_IDENT 0x4D454F57
#define QUEUE_IDENT_FREED 0x0B00B1E5
#define QUEUE_GROW_SIZE 32

#include <stdint.h>

/**
 * @brief Represents a queue, a first-in first-out (FIFO) buffer that can store various items
 */
typedef struct {
    uint32_t ident;        /**< 4-byte magic string */
    uint32_t item_size;    /**< Size of one item in the queue */
    uint32_t capacity;     /**< Current capacity of the queue */
    uint32_t num_items;    /**< Current amount of items in queue */
    uintptr_t head_offset; /**< Offset to start of queue area to data buffer in bytes */
    uintptr_t tail_offset; /**< Offset to end of queue area relative to data buffer in bytes */
    void* data;            /**< Queue data buffer */
} Queue;

/**
 * @brief Creates a new Queue
 *
 * @param item_size The size of one item in the queue
 *
 * @return The created Queue
 * @retval NULL Invalid arguments passed
 */
Queue* queue_create(uint32_t item_size);

/**
 * @brief Frees the provided Queue
 *
 * @warning This will cause an assert fail if an invalid pointer is passed
 *
 * @param queue The Queue to free
 */
void queue_free(Queue* queue);

/**
 * @brief Gets an item from a Queue and decrements the Queue's item count.
 *
 * @note This WILL gladly accept a null pointer, simply throwing the item out of the queue (i.e.
 * clocking a shift register before letting it output data)
 *
 * @warning This will cause an assert fail if an invalid pointer is passed
 *
 * @param queue The Queue to get the item from
 * @param dest The destination buffer to transfer the item to. This is nullable.
 *
 * @return Whether the operation was successful (false = queue empty)
 */
bool queue_dequeue(Queue* queue, void* dest);

/**
 * @brief Returns a pointer to the first (least recently enqueued) item in the queue
 *
 * @warning This will cause an assert fail if an invalid pointer is passed
 *
 * @param queue The Queue to get the item from
 *
 * @return The pointer to the item
 * @retval NULL The queue is empty
 */
void* queue_peek(Queue* queue);

/**
 * @brief Adds an item into the Queue
 *
 * @warning This will cause an assert fail if an invalid pointer is passed
 *
 * @param queue The Queue to add the item to
 * @param src A pointer to the buffer containing the item to add into the Queue
 */
void queue_enqueue(Queue* queue, void* src);

/**
 * @brief Grows the queue to a specified capacity
 *
 * @warning This will cause an assert fail if an invalid pointer is passed
 *
 * @param queue The queue to grow
 * @param target_capacity The target capacity
 *
 * @return Whether the operation was successful (false = queue already grew to this or bigger
 * capacity)
 */
bool queue_grow(Queue* queue, uint32_t target_capacity);

/**
 * @brief Clears the queue from any and all items
 *
 * @warning This will cause an assert fail if an invalid pointer is passed
 *
 * @note The memory space used by the queue isn't freed or reduced from this.
 *
 * @param queue The queue to clear
 */
void queue_clear(Queue* queue);

#endif
