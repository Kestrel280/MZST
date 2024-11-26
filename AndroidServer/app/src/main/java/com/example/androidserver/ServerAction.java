package com.example.androidserver;

public class ServerAction {
    public ActionType type;
    public int clientId;
    public int requesterId;
    public long timestamp;

    public enum ActionType {
        SAVE,
        LOAD,
        RESET,
        START,
        NEW,
        TRIGGER;
    }

    ServerAction(ActionType type) {
        this.type = type;
        this.clientId = -1;
    }
    public ServerAction setClientId(int id) {
        this.clientId = id;
        return this;
    }
    public ServerAction setTimestamp(long timestamp) {
        this.timestamp = timestamp;
        return this;
    }
    public ServerAction setRequesterId(int id) {
        this.requesterId = id;
        return this;
    }
}
