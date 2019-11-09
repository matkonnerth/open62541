#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "messageQueue.h"

struct Message
{
    void* data;
    struct Message* next;
};

struct MessageQueue
{
    size_t cnt;
    struct Message* messages;
    struct Message* last;
};

void MessageQueue_new(struct MessageQueue **queue)
{
    *queue = (struct MessageQueue*)calloc(sizeof(struct MessageQueue), 1);
}

void MessageQueue_enqueue(struct MessageQueue *queue, void *data)
{
    assert(queue);
    struct Message* m = (struct Message*)calloc(sizeof(struct Message), 1);
    assert(m);
    m->data=data;
    if(queue->last)
    {
        queue->last->next = m;
        queue->last = m;
    }
    else
    {
        queue->messages = m;
        queue->last = m;
    }
    queue->cnt++;
}

void *MessageQueue_dequeue(struct MessageQueue *queue)
{
    assert(queue);
    if(queue->messages)
    {
        void* data = queue->messages->data;
        queue->messages = queue->messages->next;
        queue->cnt--;
        if(queue->cnt==0)
        {
            queue->last = NULL;
        }
        return data;
    }
    return NULL;
}

