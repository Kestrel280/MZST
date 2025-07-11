#ifndef MZST_MODULEIMPL_H
#define MZST_MODULEIMPL_H

// Maximum number of states the node can store
#define MAX_CLIENT_STATES 16

// Helpers to keep track of state
typedef struct _Color { int r, g, b; } Color;
typedef struct _State { Color colorNeutral, colorTouched; } State;

/* Module implementation files must define all of the following functions, unless functions are labelled Common */

/* initMzstModule: () -> ()
    Invoked in app_main().
    Does any module-specific setup required,
        e.g. GPIO pin setup/LED setup.
*/
void initMzstModule();

/* processCommandCommon: (char*) -> ()
    Takes a command (null-terminated string) and does some action.
    Typical invocation is from messageLoop.
*/
void processCommandCommon(char* cmd);

/* processCommandSpecific: (char*) -> ()
    Fallthrough for implementation-specific processCommand().
    If no common case is found for the incoming command, it falls through to a module-specific handler.
    Note that the Common caller has already primed the command with strtok():
        this function takes the first non-common token as an argument,
        and subsequent tokens are retrieved using strtok(NULL, " ").
*/
void processCommandSpecific(char* token);

/* setColor: (int 0:255, int 0:255, int 0:255) -> ()
    Sets LEDs for the module (if they exist).
    If the module has no LEDs, this function should still be defined as a stub.
*/
void setColor(int r, int g, int b);

/* feedbackLoop: () -> ()
Thread/task function.
Performs any feedback-submodule work.
*/
void feedbackLoop();

#endif