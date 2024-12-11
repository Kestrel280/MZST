package com.example.androidserver;

import static com.example.androidserver.Server.ServerState.*;

import com.example.androidserver.Server.ServerState;

public class ActionHandler {
    // Constructor w/ dependency injection
    private final Server server;
    ActionHandler(Server server) {
        this.server = server;
    }

    // Note: LOAD action comes paired with a courseId value,
    //  which should index into a database and retrieve a Course

    // Main dispatch
    public ServerMessage processAction(ServerAction action) {
        ServerState newState;
        ServerMessage response = null;
        switch (server.getState()) {
            case DEFINE:    newState = doDefine(action); break;
            case RUN:       newState = doRun(action); break;
            case IDLE:      newState = doIdle(action); response = new ServerMessage("UNTOUCH"); break;
            case READY:     newState = doReady(action); break;
            case FINISHED:  newState = doFinished(action); break;
            default: newState = server.getState();
        }
        server.setState(newState);
        return response;
    }

    // Individual state dispatches
    public ServerState doIdle(ServerAction action) {
        switch (action.type) {
            case NEW:
                server.createNewCourse(); // Handles setting state of nodes
                return DEFINE;
            case LOAD: break;
            default: break;
        }

        return IDLE;
    }

    public ServerState doRun(ServerAction action) {
        switch (action.type) {
            case TRIGGER:
                boolean finished;
                finished = server.advanceCourse(action.clientId, action.data); // Returns true if this action finishes the course. Handles setting state of nodes (incl. SUCCESS or FAILURE of course)
                return finished ? FINISHED : RUN;
            case RESET:
                server.prepCurrentCourse();
                return READY;
            default: break;
        }

        return RUN;
    }

    public ServerState doDefine(ServerAction action) {
        switch (action.type) {
            case RESET: /* fallthrough */
            case NEW:
                // TODO prompt if want to save current course
                server.createNewCourse(); // Handles setting state of nodes
                return DEFINE;
            case LOAD: break;
            case TRIGGER:
                server.addCourseNode(action.clientId); // Handles setting state of nodes
                return DEFINE;
            case SAVE:
                server.saveCurrentCourse();
                server.prepCurrentCourse(); // Handles setting state of nodes
                return READY;
            default: break;
        }

        return DEFINE;
    }

    public ServerState doReady(ServerAction action) {
        switch (action.type) {
            case NEW:
                server.createNewCourse(); // Handles setting state of nodes
                return DEFINE;
            case LOAD: break;
            case TRIGGER: /* fallthrough */
            case START:
                boolean finished;
                finished = server.advanceCourse(action.clientId, action.data); // Returns true if this action finishes the course. Handles setting state of nodes (incl. SUCCESS or FAILURE of course)
                return finished ? FINISHED : RUN;
            default: break;
        }

        return READY;
    }

    public ServerState doFinished(ServerAction action) {
        switch (action.type) {
            case NEW:
                server.createNewCourse(); // Handles setting state of nodes
                return DEFINE;
            case RESET:
                server.prepCurrentCourse();
                return READY;
            case LOAD: break;
            default: break;
        }

        return FINISHED;
    }

    // Individual actions

}
