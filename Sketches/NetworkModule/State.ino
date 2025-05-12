#include "State.h"

// Data associated with possible states
//                                                  Standby     On-touch
StateData SD_INIT_BootStart             = StateData(&colorCyan,         nullptr);
StateData SD_INIT_WaitingWifi           = StateData(&colorRed,          nullptr);
StateData SD_INIT_WaitingServer         = StateData(&colorBlue,         nullptr);
StateData SD_INIT_Complete              = StateData(&colorGreen,        &colorWhite);
StateData SD_READYRUN_StartNode         = StateData(&colorPurple,       &colorWhite);
StateData SD_READYRUN_NotPartOfCourse   = StateData(&colorOff,          &colorRed);
StateData SD_READYRUN_NoTriggersDone    = StateData(&colorRed,          &colorWhite); // TODO when nextUp is hooked up, change on-touch to red
StateData SD_READYRUN_NextUp            = StateData(&colorBlue,         &colorWhite);
StateData SD_READYRUN_SomeTriggersDone  = StateData(&colorDimOrange,    &colorWhite); // TODO when nextUp is hooked up, change on-touch to red
StateData SD_READYRUN_AllTriggersDone   = StateData(&colorGreen,        &colorWhite); // TODO when nextUp is hooked up, change on-touch to red
StateData SD_DEFINE_SelectedNode        = StateData(&colorWhite,        nullptr);
StateData SD_DEFINE_NotInCourse         = StateData(&colorRed,          &colorBlue);
StateData SD_DEFINE_InCourse            = StateData(&colorDimOrange,    &colorWhite);
StateData SD_FINISHED_SuccessfulRun     = StateData(&colorGreen,        nullptr);
StateData SD_FINISHED_UnsuccessfulRun   = StateData(&colorOff,          nullptr);

// Update global 'state' and 'stateData' variables
// GLOBALS: state, stateData
void setState(State newState) {
  switch(newState) {
    case INIT_WaitingWifi:          stateData = &SD_INIT_WaitingWifi;          break;   
    case INIT_WaitingServer:        stateData = &SD_INIT_WaitingServer;        break;   
    case INIT_Complete:             stateData = &SD_INIT_Complete;             break;       
    case READYRUN_StartNode:        stateData = &SD_READYRUN_StartNode;        break;
    case READYRUN_NotPartOfCourse:  stateData = &SD_READYRUN_NotPartOfCourse;  break;
    case READYRUN_NoTriggersDone:   stateData = &SD_READYRUN_NoTriggersDone;   break;
    case READYRUN_SomeTriggersDone: stateData = &SD_READYRUN_SomeTriggersDone; break;
    case READYRUN_AllTriggersDone:  stateData = &SD_READYRUN_AllTriggersDone;  break;
    case DEFINE_SelectedNode:       stateData = &SD_DEFINE_SelectedNode;       break;
    case DEFINE_NotInCourse:        stateData = &SD_DEFINE_NotInCourse;        break;
    case DEFINE_InCourse:           stateData = &SD_DEFINE_InCourse;           break;
    case FINISHED_SuccessfulRun:    stateData = &SD_FINISHED_SuccessfulRun;    break;
    case FINISHED_UnsuccessfulRun:  stateData = &SD_FINISHED_UnsuccessfulRun;  break;
  }
  Serial.printf("SetState (previous %d, new %d)\n", state, newState);
  state = newState;
  writeLed(stateData->colorIdle);
}

// Translate from a state name to the appropriate state object
State parseStateName(std::string stateName) {
  if      (stateName == "READYRUN_StartNode")         return READYRUN_StartNode;
  else if (stateName == "READYRUN_NotPartOfCourse")   return READYRUN_NotPartOfCourse;
  else if (stateName == "READYRUN_NoTriggersDone")    return READYRUN_NoTriggersDone;
  else if (stateName == "READYRUN_NextUp")            return READYRUN_NextUp;
  else if (stateName == "READYRUN_SomeTriggersDone")  return READYRUN_SomeTriggersDone;
  else if (stateName == "READYRUN_AllTriggersDone")   return READYRUN_AllTriggersDone;
  else if (stateName == "DEFINE_SelectedNode")        return DEFINE_SelectedNode;
  else if (stateName == "DEFINE_NotInCourse")         return DEFINE_NotInCourse;
  else if (stateName == "DEFINE_InCourse")            return DEFINE_InCourse;
  else if (stateName == "FINISHED_SuccessfulRun")     return FINISHED_SuccessfulRun;
  else if (stateName == "FINISHED_UnsuccessfulRun")   return FINISHED_UnsuccessfulRun;
}

