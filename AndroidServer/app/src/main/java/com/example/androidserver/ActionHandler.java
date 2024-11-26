package com.example.androidserver;

import com.example.androidserver.Server.ServerState;

public class ActionHandler {
    // Constructor w/ dependency injection
    private final Server server;
    ActionHandler(Server server) {
        this.server = server;
    }

    // Main dispatch
    public ServerState processAction(ServerAction action) {
        ServerState newState;
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
