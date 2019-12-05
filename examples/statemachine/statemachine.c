#include "statemachine.h"
#include "messageQueue.h"
#include "stdlib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_SUB_IDS 10

struct Statemachine
{
    enum State state;
    struct MessageQueue* externalEvents;
    struct MessageQueue* generatedEvents;
    size_t subCnt;
    enum MessageId lastMessages[MAX_SUB_IDS];
};

void Statemachine_new(struct Statemachine **sm)
{
    *sm = (struct Statemachine*)calloc(sizeof(struct Statemachine), 1);
    assert(*sm);
    MessageQueue_new(&(*sm)->externalEvents);
    MessageQueue_new(&(*sm)->generatedEvents);
    (*sm)->state = STATE_MANUAL;
}

int Statemachine_addSubstatemachine(struct Statemachine*sm)
{
    return (int)sm->subCnt++;
}

static void clearLastMessages(struct Statemachine* sm)
{
    for(size_t i=0; i<sm->subCnt; i++)
    {
        sm->lastMessages[i] = UNDEFINED;
    }
}

static bool allTransitionEventsAvailable(struct Statemachine* sm)
{
    for(size_t i=0; i<sm->subCnt; i++)
    {
        if(sm->lastMessages[i]==UNDEFINED)
        {
            return false;
        }
    }
    return true;
}

static bool isTransitionDone(struct Statemachine* sm, enum MessageId expected)
{
    for(size_t i = 0; i < sm->subCnt; i++) {
        if(sm->lastMessages[i] != expected) {
            return false;
        }
    }
    return true;
}

static void addResult(struct Statemachine *sm, struct Message m)
{
    sm->lastMessages[m.subId] = m.id;
}

void Statemachine_process(struct Statemachine *sm)
{
    struct Message* in = (struct Message*) MessageQueue_dequeue(sm->externalEvents);
    while(in)
    {
        switch(sm->state)
        {
            case STATE_MANUAL:
                if(in->id == IN_REQUEST_AUTOMATIC) {
                    printf("request to automatic\n");
                    for(size_t i=0; i<sm->subCnt; i++)
                    {
                        struct Message *data =
                            (struct Message *)malloc(sizeof(struct Message));
                        data->id = OUT_REQUEST_AUTOMATIC;
                        data->subId = (int)i;
                        MessageQueue_enqueue(sm->generatedEvents, data);
                    }
                    clearLastMessages(sm);
                    sm->state = STATE_REQUEST_AUTOMATIC;
                }
                break;
            case STATE_REQUEST_AUTOMATIC:
                if(in->id == IN_TRANSITION_AUTOMATIC) {
                    addResult(sm, *in);
                    if(allTransitionEventsAvailable(sm) && isTransitionDone(sm, IN_TRANSITION_AUTOMATIC))
                    {
                        clearLastMessages(sm);
                        printf("transition to automatic finished\n");
                        struct Message *data =
                            (struct Message *)malloc(sizeof(struct Message));
                        data->id = OUT_TRANSITION_AUTOMATIC_FINISHED;
                        MessageQueue_enqueue(sm->generatedEvents, data);
                        sm->state = STATE_AUTOMATIC;
                    }
                }
                break;
            case STATE_AUTOMATIC:
                if(in->id == IN_REQUEST_MANUAL) {
                    printf("request to manual\n");
                    for(size_t i = 0; i < sm->subCnt; i++) {
                        struct Message *data =
                            (struct Message *)malloc(sizeof(struct Message));
                        data->id = OUT_REQUEST_MANUAL;
                        data->subId = (int)i;
                        MessageQueue_enqueue(sm->generatedEvents, data);
                    }
                    clearLastMessages(sm);
                    sm->state = STATE_REQUEST_MANUAL;
                }
                break;
            case STATE_REQUEST_MANUAL:
                if(in->id == IN_TRANSITION_MANUAL) {
                    addResult(sm, *in);
                    if(allTransitionEventsAvailable(sm) &&
                       isTransitionDone(sm, IN_TRANSITION_MANUAL)) {
                        clearLastMessages(sm);
                        printf("transition to manual finished\n");
                        struct Message *data =
                            (struct Message *)malloc(sizeof(struct Message));
                        data->id = OUT_TRANSITION_MANUAL_FINISHED;
                        MessageQueue_enqueue(sm->generatedEvents, data);
                        sm->state = STATE_MANUAL;
                    }
                }
                break;

            default:
                assert(false);
        }
        in = (struct Message *)MessageQueue_dequeue(sm->externalEvents);
    }
}

void
Statemachine_setInputEvent(struct Statemachine *sm, struct Message in)
{
    struct Message* data = (struct Message*)malloc(sizeof(struct Message));
    assert(data);
    *data = in;
    MessageQueue_enqueue(sm->externalEvents, data);
}

struct Message
Statemachine_getOutputEvent(struct Statemachine *sm)
{
    struct Message* ev = (struct Message*) MessageQueue_dequeue(sm->generatedEvents);
    if(ev)
        return *ev;
    struct Message m = {OUT_REQUEST_EMPTY, 0};
    return m;
}

enum State Statemachine_getState(const struct Statemachine* sm)
{
    return sm->state;
}
