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
    (*sm)->state = STATE_MANUAL;
}

void Statemachine_process(struct Statemachine *sm)
{
    enum IncomingEvent* ev = (enum IncomingEvent*) MessageQueue_dequeue(sm->externalEvents);
    while(ev)
    {
        switch(sm->state)
        {
            case STATE_MANUAL:
                if(*ev == IN_REQUEST_AUTOMATIC) {
                    printf("request to automatic\n");
                    enum OutgoingEvent *data =
                        (enum OutgoingEvent*)malloc(sizeof(enum OutgoingEvent));
                    *data = OUT_REQUEST_AUTOMATIC;
                    MessageQueue_enqueue(sm->generatedEvents, data);
                    sm->state = STATE_REQUEST_AUTOMATIC;
                }
                break;
            case STATE_REQUEST_AUTOMATIC:
                if(*ev == IN_TRANSITION_AUTOMATIC) {
                    printf("transition to automatic\n");
                    sm->state = STATE_AUTOMATIC;
                }
                break;
            case STATE_AUTOMATIC:
                if(*ev==IN_REQUEST_MANUAL)
                {
                    printf("request for manual\n");
                    enum OutgoingEvent *data =
                        (enum OutgoingEvent *)malloc(sizeof(enum OutgoingEvent));
                    *data = OUT_REQUEST_MANUAL;
                    MessageQueue_enqueue(sm->generatedEvents, data);
                    sm->state=STATE_REQUEST_MANUAL;
                }
                break;
            case STATE_REQUEST_MANUAL:
                if(*ev == IN_TRANSITION_MANUAL) {
                    printf("transition to manual\n");
                    sm->state = STATE_MANUAL;
                }
                break;

            default:
                assert(false);
        }
        ev = (enum IncomingEvent *)MessageQueue_dequeue(sm->externalEvents);
    }
}

void
Statemachine_setInputEvent(struct Statemachine *sm, enum IncomingEvent ev)
{
    enum IncomingEvent* data = (enum IncomingEvent*)malloc(sizeof(enum IncomingEvent));
    assert(data);
    *data = ev;
    MessageQueue_enqueue(sm->externalEvents, data);
}

enum OutgoingEvent
Statemachine_getOutputEvent(struct Statemachine *sm)
{
    enum OutgoingEvent* ev = (enum OutgoingEvent*) MessageQueue_dequeue(sm->generatedEvents);
    if(ev)
        return *ev;
    return OUT_REQUEST_EMPTY;
}

enum State Statemachine_getState(const struct Statemachine* sm)
{
    return sm->state;
}