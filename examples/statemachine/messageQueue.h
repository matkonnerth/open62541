#ifndef MESSAGEQUEUE
#define MESSSAGEQUEUE

struct MessageQueue;

void MessageQueue_new(struct MessageQueue** queue);
void MessageQueue_delete(struct MessageQueue** queue);
void MessageQueue_enqueue(struct MessageQueue* queue, void* data);
void* MessageQueue_dequeue(struct MessageQueue* queue);


#endif