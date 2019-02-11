
#include <stdbool.h>
#include "statemachine.h"



static state currentState = STOPPED;

typedef state (*transitionHandler)(void);

typedef struct Transition Transition;

struct Transition{
    bool execute;
    state to;
    transitionHandler handler;
    Transition *next;
};

typedef struct
{
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

static Transition t_stopped_to_running = {false, RUNNING, transitionHandler_Stopped_to_Running, 0};
static Transition t_running_to_stopped = {false, STOPPED, transitionHandler_Running_to_Stopped, 0};
static Transition t_empty = {false, ERROR, 0, 0};
static StateType stateMachine[] = {{&t_stopped_to_running}, {&t_running_to_stopped}, {&t_empty}};


bool request_from_to(state from, state to)
{
    if((currentState!=from))
        return false;
    Transition *t = stateMachine[from].transitions;
    while (t!=0)
    {
        if(t->to==to)
        {
            t->execute = true;
            return true;
        }
        t = t->next;
    }
    return false;
}

static void handleState(state s)
{
    //entry action
   
    //cyclic parts


    //check transitions
    Transition *t = stateMachine[s].transitions;
    while (t != 0) 
    {
        if (t->execute == true)
        {
            t->execute = false;
            state newState = t->handler();
            if(currentState!=newState)
            {
                currentState = newState;
                //notify state change in here
            }
        }
        t = t->next;
    }
}

void run() { handleState(currentState); }
int getCurrentState() { return currentState; }