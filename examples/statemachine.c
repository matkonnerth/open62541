
#include <stdbool.h>
#include "statemachine.h"



state currentState = STOPPED;

typedef state (*transitionHandler)(void);

typedef struct
{
    bool execute;
    transitionHandler handler;
} Transition;

typedef struct
{    
    int noOfTransistions;
    Transition* transitions;
} StateType;

static state transitionHandler_Stopped_to_Running(void)
{
    return RUNNING; 
}

static state transitionHandler_Running_to_Stopped(void) 
{ 
    return STOPPED; 
}

Transition t_stopped_to_running = {false, transitionHandler_Stopped_to_Running};
Transition t_running_to_stopped = {false, transitionHandler_Running_to_Stopped};
Transition t_empty = {false, 0};
StateType stateMachine[] = {{1, &t_stopped_to_running}, {1, &t_running_to_stopped}, {0, &t_empty}};

bool call_stopped_to_running() {
    if (!(currentState == STOPPED))
        return false;
    stateMachine[STOPPED].transitions[0].execute = true;
    return true;
}

bool call_running_to_stopped() {
    if (!(currentState == RUNNING))
        return false;
    stateMachine[RUNNING].transitions[0].execute = true;
    return true;
}

static void handleState(state s)
{
    //entry action
   
    //cyclic parts


    //check transitions
    for (int t = 0; t < stateMachine[s].noOfTransistions; t++)
    {
        if(stateMachine[s].transitions[t].execute)
        {
            stateMachine[s].transitions[t].execute = false;
            state newState = stateMachine[s].transitions[t].handler();
            currentState = newState;
        }
    }
}

void run() { handleState(currentState); }
int getCurrentState() { return currentState; }