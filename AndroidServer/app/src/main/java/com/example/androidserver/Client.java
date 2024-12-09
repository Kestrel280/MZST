package com.example.androidserver;

import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.ArrayList;

import java.util.List;

public class Client {
    public static final int USER = 1;
    public static final int NODE = 2;
    public static final int TRANSMITTER = 3;

    public int type;
    private static int __debug_id = 0;
    public int id;
    private Socket socket;
    private BufferedReader reader;
    private PrintWriter writer;


    Client(Socket socket) {
        this(socket, -1);
    }
    Client(Socket socket, int type) {
        try {
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            writer = new PrintWriter(socket.getOutputStream(), true);
            id = -1; // id will be set when reading registration message
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        this.type = type;
    }

    public void setType(int newType) {
        this.type = newType;
    }

    public ClientMessage readMessage() {
        char[] line = new char[ClientMessage.PacketSize];
        ClientMessage msg = null;
        try {
            if (reader.ready()) {
                reader.read(line, 0, ClientMessage.PacketSize);
                msg = new ClientMessage(line);
                Log.i("server", String.format("Received msg from client %d of type %d: code=%d, id=%d, data=%d", this.id, this.type, msg.code, msg.id, msg.data));
                MainActivity.debugMsgToAppView(String.format("Received msg from client %d of type %d: code=%d, id=%d, data=%d", this.id, this.type, msg.code, msg.id, msg.data), "debug");
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return msg;
    }

    public List<ClientMessage> readMessageQueue() {
        List<ClientMessage> messages = new ArrayList<>();

        try {
            while (reader.ready())  {
                messages.add(this.readMessage());
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return messages;
    }

    public void sendMessage(ServerMessage msg) {
        if (!msg.text.isEmpty()) {
            Log.i("server", String.format("Sending message to client %d of type %d: %s", this.id, this.type, msg.text));
            writer.println(msg.text);
        }
    }
}
