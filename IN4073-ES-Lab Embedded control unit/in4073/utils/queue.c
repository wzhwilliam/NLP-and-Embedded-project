#include "queue.h"

void init_queue(Queue *queue)
{
	queue->first = 0;
	queue->last = QUEUE_SIZE - 1;
	queue->count = 0;
}

/**
 * @brief Function to put one byte into the queue.
 * @param Queue* Pointer to the queue
 * @param uint8_t* Pointer to the item we want to place in the queue
 * @return false if there's no free spot in the queue, else true
 * @author modified by Kristóf
 */
bool enqueue(Queue *queue, uint8_t item)
{
	if (queue->count < QUEUE_SIZE)
	{
		queue->last = (queue->last + 1) % QUEUE_SIZE;
		queue->items[queue->last] = item;
		queue->count += 1;
		return true;
	}
	
	return false;
}

uint8_t dequeue(Queue *queue)
{
	uint8_t first_item = queue->items[queue->first];
	queue->first = (queue->first + 1) % QUEUE_SIZE;
	queue->count -= 1;

	return first_item;
}
