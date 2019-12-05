#ifndef STATEMACHINE_H
#define STATEMACHINE_H


//this events can be sent to the statemachine
enum MessageId {
    UNDEFINED,
    IN_REQUEST_AUTOMATIC,
    IN_REQUEST_MANUAL,
    IN_TRANSITION_AUTOMATIC,
    IN_TRANSITION_MANUAL,
    OUT_REQUEST_AUTOMATIC,
    OUT_REQUEST_MANUAL,
    OUT_REQUEST_EMPTY,
    OUT_TRANSITION_AUTOMATIC_FINISHED,
    OUT_TRANSITION_MANUAL_FINISHED
};

struct Message
{
    enum MessageId id;
    int subId;
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
void Statemachine_setInputEvent(struct Statemachine* sm, struct Message);
struct Message Statemachine_getOutputEvent(struct Statemachine* sm);
enum State Statemachine_getState(const struct Statemachine* sm);
int Statemachine_addSubstatemachine(struct Statemachine* sm);

#endif
