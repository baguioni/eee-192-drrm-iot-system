#ifndef SRC_MESSAGE_QUEUE_H_
#define SRC_MESSAGE_QUEUE_H_

#define MAX_QUEUE_SIZE 100

typedef enum  {
    VOLTAGE,
    HUMIDITY,
    TEMPERATURE,
    SMOKE
} Sensor;

typedef enum {
    EnqueueResult_Success,
    EnqueueResult_Full,
} EnqueueResult;

typedef enum {
    DequeueResult_Success,
    DequeueResult_Empty,
} DequeueResult;

typedef struct {
    float data[MAX_QUEUE_SIZE];
    int front, rear;
} Queue;

Queue* createQueue();
int isEmpty(Queue* queue);
int isFull(Queue* queue);
EnqueueResult enqueue(Queue* queue, int item);
DequeueResult dequeue(Queue* queue, int* item);
uint16_t peek(Queue* queue);

#endif /* SRC_MESSAGE_QUEUE_H_ */
