package com.example.androidserver;

import android.os.Bundle;
import android.os.Message;
import android.util.Log;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.Socket;

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

    public void readMessageQueue() {
        String line = "";

        try {
            while (reader.ready())  {
                line = reader.readLine();
                processMessage(line);
                MainActivity.debugMsgToAppView(String.format("Client %d says: %s", id, line));
//                Message msg = MainActivity.uiMessageHandler.obtainMessage();
 //               Bundle bundle = new Bundle();
  //              bundle.putCharSequence("MESSAGE", String.format("Client %d msg: %s", id, line));
   //             msg.setData(bundle);
    //            MainActivity.uiMessageHandler.sendMessage(msg);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    // TODO maybe have this return the response instead of sending it right in here
    private void processMessage(String msg) {
        Log.i("server", String.format("Handler received message from client %d: %s", id, msg));
        String response = "";
        switch (msg) {
            case "TRIGGERED": {
                response = "RESET";
                break;
            }
            default: break;
        }
        if (!response.isEmpty()) {
            Log.i("server", String.format("Server sending response to client %d: %s", id, response));
            writer.println(response);
        }
    }
}
