/* Module implementation files must define all of the following functions, unless functions are labelled Common */

/* initMzstModule: () -> ()
    Invoked in app_main().
    Does any module-specific setup required,
        e.g. GPIO pin setup/LED setup.
*/
void initMzstModule();

/* processCommand: (char*) -> ()
    Takes a command (null-terminated string) and does some action.
    Typical invocation is from messageLoop.
*/
void processCommand(char* cmd);

/* processCommandCommon: (char*) -> ()
    Fallthrough for implementation-specific processCommand().
    If no case is found for the incoming command, it falls through to this.
*/

void processCommandCommon();
/* setColor: (int 0:255, int 0:255, int 0:255) -> ()
    Sets LEDs for the module (if they exist).
    If the module has no LEDs, this function should still be defined as a stub.
*/
void setColor(int r, int g, int b);