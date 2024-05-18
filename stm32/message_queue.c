#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "message_queue.h"

Queue* createQueue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->front = -1;
    queue->rear = -1;
    return queue;
}

int isEmpty(Queue* queue) {
    return queue->front == -1;
}

int isFull(Queue* queue) {
    return (queue->rear + 1) % MAX_QUEUE_SIZE == queue->front;
}

EnqueueResult enqueue(Queue* queue, int item) {
    if (isFull(queue)) {
        // Queue is full.
        return EnqueueResult_Full;
    }
    if (isEmpty(queue)) {
        queue->front = 0;
        queue->rear = 0;
    } else {
        queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    }
    queue->data[queue->rear] = item;
    return EnqueueResult_Success;
}

DequeueResult dequeue(Queue* queue, int* item) {
    if (isEmpty(queue)) {
        // Queue is empty.
        return DequeueResult_Empty;
    }
    *item = queue->data[queue->front];
    if (queue->front == queue->rear) {
        queue->front = -1;
        queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    }
    return DequeueResult_Success;
}

uint16_t peek(Queue* queue) {
    if (isEmpty(queue)) {
        // Queue is empty.
        return UINT16_MAX;
    }
    return queue->data[queue->front];
}
