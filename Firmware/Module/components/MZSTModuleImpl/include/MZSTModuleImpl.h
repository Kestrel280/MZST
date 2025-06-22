/* Module implementation files must define all of the following functions */

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

/* setColor: (int 0:255, int 0:255, int 0:255) -> ()
    Sets LEDs for the module (if they exist).
    If the module has no LEDs, this function should still be defined as a stub.
*/
void setColor(int r, int g, int b);