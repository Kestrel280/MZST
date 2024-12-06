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
    private static int __debug_id = 0;
    public int id;
    private Socket socket;
    private BufferedReader reader;
    private PrintWriter writer;

    Client(Socket socket) {
        try {
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            writer = new PrintWriter(socket.getOutputStream(), true);
            id = __debug_id++; // TODO read id from client
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public ClientMessage readMessage() {
        char[] line = new char[ClientMessage.PacketSize];
        ClientMessage msg = null;
        try {
            if (reader.ready()) {
                reader.read(line, 0, ClientMessage.PacketSize);
                msg = new ClientMessage(line);
                Log.i("server", String.format("Received msg: type=%d, id=%d, ts=%dms", msg.type, msg.id, msg.timestamp));
                MainActivity.debugMsgToAppView(String.format("Received msg: type=%d, id=%d, ts=%dms", msg.type, msg.id, msg.timestamp));
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
            Log.i("server", String.format("Sending message to client %d: %s", id, msg.text));
            writer.println(msg.text);
        }
    }
}
