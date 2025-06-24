#ifndef MZST_ADMINLISTENER_H
#define MZST_ADMINLISTENER_H

#define ADMIN_RECEIVE_BUF_SIZE 1024
#define ADMIN_LISTEN_PORT 7777

/* adminListenerLoop: (*processCommandFunction)(char*) -> ()
Thread/task function.
Starts a listener socket on port [TODO], and runs the following loop:
    1. Accept a connection on this port
    2. Accept data until receiving a newline (TODO or timeout)
    3. Parse the data (replace newline with null byte) and pass to processCommandFunction
Note that the prototype is of type void fn(void), but the implementation should expect a function pointer to processCommandFunction(char*)!
*/
void adminListenerLoop();

#endif