#ifndef STATE_H
#define STATE_H

#include "Color.h"

// Possible states for a module
typedef enum _State {
  INIT_BootStart,             // State set immediately upon module boot
  INIT_WaitingWifi,           // While waiting for wifi connection
  INIT_WaitingServer,         // While waiting for server connection
  INIT_Complete,              // Connected to server, awaiting instructions
  READYRUN_StartNode,         // Starting node for a course. When it's touched, server will move into RUN state
  READYRUN_NotPartOfCourse,   // In ready/run mode: the node IS NOT part of the active course
  READYRUN_NoTriggersDone,    // In ready/run mode: the node is part of the active course, but has not yet been triggered a single time
  READYRUN_NextUp,            // In ready/run mode: the node is part of the active course, and is the next node which must be triggered
  READYRUN_SomeTriggersDone,  // In ready/run mode: the node is part of the active course, and has been triggered, but the node re-appears later in the course so it will need to be triggered again
  READYRUN_AllTriggersDone,   // In ready/run mode: the node IS part of the active course, and has been triggered, and does not appear later in the course
  DEFINE_SelectedNode,        // In edit mode: the most-recently selected node
  DEFINE_NotInCourse,         // In edit mode: a node which has not been added to the course, but is able to be added
  DEFINE_InCourse,            // In edit mode: a node which has been added to the course, and is able to be added again
  FINISHED_SuccessfulRun,     // In finished mode: the run was successful
  FINISHED_UnsuccessfulRun    // In finished mode: the run was unsuccessful
} State;

struct StateData {
  Color* colorIdle;
  Color* colorOnTouch; // If null, non-receptive to touch
  StateData(Color* colorIdle, Color* colorOnTouch) {
    this->colorIdle = colorIdle;
    this->colorOnTouch = colorOnTouch;
  }
};

void setState(State newState);
State parseStateName(std::string stateName);

#endif
