package com.example.androidserver;

import android.os.Bundle;
import android.os.Message;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.Socket;

public class Client {
    private static int __debug_id = 0;
    public int id;
    private Socket socket;
    private BufferedReader reader;

    Client(Socket socket) {
        try {
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            id = __debug_id++; // TODO read id from client
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public void readMessageQueue() {
        String line = "";

        try {
            while (!(line = reader.readLine()).isEmpty()) {
                Message msg = MainActivity.uiMessageHandler.obtainMessage();
                Bundle bundle = new Bundle();
                bundle.putCharSequence("MESSAGE", line);
                msg.setData(bundle);
                MainActivity.uiMessageHandler.sendMessage(msg);
                //Log.i("client", String.format("Client %d received message: " + line, id));
            }
        } catch (IOException e) {
                throw new RuntimeException(e);
            }
    }
}
