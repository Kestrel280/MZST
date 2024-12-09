package com.example.androidserver;

import android.util.Log;

import java.util.ArrayList;

public class Course {
    private int id; // Course id; not directly modifiable by user, should only be accessed/changed upon saving/loading from a database
    public String name;
    public ArrayList<Integer> nodeSequence;
    public boolean ordered; // If true, the course must be done in nodeSequence order; if false, any order
    private boolean editing; // If true, the course is in "editing" mode and can be modified

    public Course() {
        this.id = -1;
        this.name = "Unnamed Course";
        this.ordered = true;
        this.nodeSequence = new ArrayList<>();
        this.editing = true;
    }

    public Course addNode(int nodeId) {
        if (this.editing) {
            this.nodeSequence.add(nodeId);
        }
        Log.i("course", String.format("Course %d added node %d; course is now", this.id, nodeId) + this.nodeSequence.toString());
        return this;
    }

    public Course finishEditing() {
        this.editing = false;

        // Remove any contiguous duplicates
        for (int i = this.nodeSequence.size() - 1; i > 0; i--) {
            if (this.nodeSequence.get(i).equals(this.nodeSequence.get(i - 1))) {
                this.nodeSequence.remove(i);
            }
        }
        MainActivity.debugMsgToAppView("Finished editing course with sequence " + this.nodeSequence.toString(), "debug");
        Log.i("course", String.format("Finished editing course %d: sequence is ", this.id) + this.nodeSequence.toString());
        return this;
    }

    public Course setName(String newName) {
        this.name = newName;
        return this;
    }

}
