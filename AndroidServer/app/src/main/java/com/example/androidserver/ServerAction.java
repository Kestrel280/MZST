package com.example.androidserver;

public class ServerAction {
    public ActionType type;
    public int clientId;
    public int requesterId;
    public long timestamp;
    public int courseId;

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
        this.requesterId = -1;
        this.courseId = -1;
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
    public ServerAction setCourseId(int id) {
        this.courseId = id;
        return this;
    }
}
