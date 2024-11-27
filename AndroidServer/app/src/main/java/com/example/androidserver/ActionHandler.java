package com.example.androidserver;

import android.util.Log;

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
    public ServerState processAction(ServerAction action) {
        ServerState newState;
        Log.i("actionhandler", "Action handler processing action of type " + action.type.toString());
        switch (server.getState()) {
            case DEFINE:    newState = doDefine(action); break;
            case RUN:       newState = doRun(action); break;
            case IDLE:      newState = doIdle(action); break;
            case READY:     newState = doReady(action); break;
            case FINISHED:  newState = doFinished(action); break;
            default: newState = server.getState();
        }
        return newState;
    }

    // Individual state dispatches
    public ServerState doIdle(ServerAction action) {
        switch (action.type) {
            case NEW: break;
            case LOAD: break;
            default: break;
        }

        return ServerState.IDLE;
    }

    public ServerState doRun(ServerAction action) {
        switch (action.type) {
            case TRIGGER: break;
            case RESET: break;
            default: break;
        }

        return ServerState.RUN;
    }

    public ServerState doDefine(ServerAction action) {
        switch (action.type) {
            case NEW: break;
            case LOAD: break;
            case RESET: break;
            case TRIGGER: break;
            case SAVE: break;
            default: break;
        }

        return ServerState.DEFINE;
    }

    public ServerState doReady(ServerAction action) {
        switch (action.type) {
            case NEW: break;
            case LOAD: break;
            case START: break;
            default: break;
        }

        return ServerState.READY;
    }

    public ServerState doFinished(ServerAction action) {
        switch (action.type) {
            case NEW: break;
            case RESET: break;
            case LOAD: break;
            default: break;
        }

        return ServerState.FINISHED;
    }

    // Individual actions

}
