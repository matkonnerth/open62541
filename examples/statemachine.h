#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

typedef enum { STOPPED = 0, RUNNING = 1, ERROR = 2 } state;

void run(void);
int getCurrentState(void);
bool call_stopped_to_running(void);
bool call_running_to_stopped(void);

#endif