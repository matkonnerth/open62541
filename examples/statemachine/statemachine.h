#ifndef STATEMACHINE_H
#define STATEMACHINE_H

//this events can be sent to the statemachine
enum IncomingEvent
{
    IN_REQUEST_AUTOMATIC,
    IN_REQUEST_MANUAL,
    IN_TRANSITION_AUTOMATIC,
    IN_TRANSITION_MANUAL
};

enum OutgoingEvent
{
    OUT_REQUEST_AUTOMATIC,
    OUT_REQUEST_MANUAL,
    OUT_REQUEST_EMPTY
};

enum State
{
    STATE_MANUAL,
    STATE_REQUEST_AUTOMATIC,
    STATE_AUTOMATIC,
    STATE_REQUEST_MANUAL
};
struct Statemachine;

void Statemachine_new(struct Statemachine** sm);
void Statemachine_process(struct Statemachine* sm);
void Statemachine_setInputEvent(struct Statemachine* sm, enum IncomingEvent ev);
enum OutgoingEvent Statemachine_getOutputEvent(struct Statemachine* sm);
enum State Statemachine_getState(const struct Statemachine* sm);

#endif
