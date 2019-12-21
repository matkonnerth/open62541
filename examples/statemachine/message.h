#pragma once

// this events can be sent to the statemachine
enum MessageId {
    EMPTY,
    UNDEFINED,
    IN_REQUEST_AUTOMATIC,
    IN_REQUEST_MANUAL,
    IN_TRANSITION_AUTOMATIC,
    IN_TRANSITION_MANUAL,
    OUT_REQUEST_AUTOMATIC,
    OUT_REQUEST_MANUAL,
    OUT_TRANSITION_AUTOMATIC_FINISHED,
    OUT_TRANSITION_MANUAL_FINISHED
};

struct Message {
    enum MessageId id;
    int subId;
};
