#ifndef STATEMACHINE_H
#define STATEMACHINE_H
#include "message.h"

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
void Statemachine_setInputEvent(struct Statemachine* sm, struct Message);
struct Message Statemachine_getOutputEvent(struct Statemachine* sm);
enum State Statemachine_getState(const struct Statemachine* sm);
int Statemachine_addSubstatemachine(struct Statemachine* sm);

#endif
