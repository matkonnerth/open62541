#ifndef MESSAGEQUEUE
#define MESSSAGEQUEUE
#include "message.h"
struct MessageQueue;

void MessageQueue_new(struct MessageQueue** queue);
void MessageQueue_delete(struct MessageQueue** queue);
void MessageQueue_enqueue(struct MessageQueue* queue, const struct Message* m);
struct Message MessageQueue_dequeue(struct MessageQueue* queue);


#endif
