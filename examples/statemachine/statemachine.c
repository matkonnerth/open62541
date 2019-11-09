#include "statemachine.h"
#include "messageQueue.h"
#include "stdlib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

struct Statemachine
{
    enum State state;
    struct MessageQueue* externalEvents;
    struct MessageQueue* generatedEvents;
};

void Statemachine_new(struct Statemachine **sm)
{
    *sm = (struct Statemachine*)calloc(sizeof(struct Statemachine), 1);
    assert(*sm);
    MessageQueue_new(&(*sm)->externalEvents);
    MessageQueue_new(&(*sm)->generatedEvents);
    (*sm)->state = MANUAL;
}

void Statemachine_process(struct Statemachine *sm)
{
    enum ExternalEvent* ev = (enum ExternalEvent*) MessageQueue_dequeue(sm->externalEvents);
    while(ev)
    {
        switch(sm->state)
        {
            case MANUAL:
                if(*ev==REQUEST_AUTOMATIC)
                {
                    printf("transition to automatic\n");
                    sm->state=AUTOMATIC;
                }
                break;
            case AUTOMATIC:
                if(*ev==REQUEST_MANUAL)
                {
                    printf("transition to manual\n");
                    sm->state=MANUAL;
                }
                break;
            default:
                assert(false);
        }
        ev = (enum ExternalEvent *)MessageQueue_dequeue(sm->externalEvents);
    }
}

void
Statemachine_setInputEvent(struct Statemachine *sm, enum ExternalEvent ev)
{
    enum ExternalEvent* data = (enum ExternalEvent*)malloc(sizeof(enum ExternalEvent));
    assert(data);
    *data = ev;
    MessageQueue_enqueue(sm->externalEvents, data);
}

enum State Statemachine_getState(const struct Statemachine* sm)
{
    return sm->state;
}