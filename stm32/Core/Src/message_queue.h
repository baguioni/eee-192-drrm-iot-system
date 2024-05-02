#ifndef SRC_MESSAGE_QUEUE_H_
#define SRC_MESSAGE_QUEUE_H_

#define MAX_QUEUE_SIZE 100

enum Sensor {
    VOLTAGE,
    HUMIDITY,
    SMOKE
};

typedef enum {
    EnqueueResult_Success,
    EnqueueResult_Full,
} EnqueueResult;

typedef enum {
    DequeueResult_Success,
    DequeueResult_Empty,
} DequeueResult;

typedef struct {
    uint16_t data[MAX_QUEUE_SIZE];
    int front, rear;
} Queue;

Queue* createQueue();
int isEmpty(Queue* queue);
int isFull(Queue* queue);
EnqueueResult enqueue(Queue* queue, uint16_t item);
DequeueResult dequeue(Queue* queue, uint16_t* item);
uint16_t peek(Queue* queue);

#endif /* SRC_MESSAGE_QUEUE_H_ */
