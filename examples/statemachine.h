#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

typedef enum { STOPPED = 0, RUNNING = 1, ERROR = 2 } state;

void run(void);
int getCurrentState(void);
bool request_from_to(state from, state to);

#endif