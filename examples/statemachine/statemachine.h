#ifndef STATEMACHINE_H
#define STATEMACHINE_H

//this events can be sent to the statemachine
enum ExternalEvent
{
    REQUEST_AUTOMATIC,
    REQUEST_MANUAL
};

enum State
{
    MANUAL,
    AUTOMATIC
};
struct Statemachine;

void Statemachine_new(struct Statemachine** sm);
void Statemachine_process(struct Statemachine* sm);
void Statemachine_setInputEvent(struct Statemachine* sm, enum ExternalEvent ev);
enum State Statemachine_getState(const struct Statemachine* sm);

#endif
