#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "messageQueue.h"

#define CAPACITY 16

struct MessageQueue
{
    size_t readIdx;
    size_t writeIdx;
    struct Message messages[CAPACITY];
};

void MessageQueue_new(struct MessageQueue **queue)
{
    *queue = (struct MessageQueue*)calloc(sizeof(struct MessageQueue), 1);
}

void MessageQueue_enqueue(struct MessageQueue *queue, const struct Message *m)
{
    assert(queue);
    assert(m);
    size_t in = queue->writeIdx - queue->readIdx;
    if(in==CAPACITY)
        return;

    size_t i = (queue->writeIdx)++ & (CAPACITY-1);
    queue->messages[i]= *m;
}

struct Message MessageQueue_dequeue(struct MessageQueue *queue)
{
    assert(queue);
    size_t in = queue->writeIdx - queue->readIdx;
    if(in==0)
    {
        struct Message empty = {EMPTY, 0};
        return empty;
    }
    size_t i = (queue->readIdx)++ & (CAPACITY - 1);
    return queue->messages[i];
}

