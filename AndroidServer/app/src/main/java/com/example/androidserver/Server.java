package com.example.androidserver;

import android.util.Log;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Server implements Runnable {
    public static final int PORT = 5000;

    private Socket          socket = null;
    private ServerSocket    server = null;
    private DataInputStream in     = null;

    private void _Server(int port) {
        try {
            server = new ServerSocket(port);
            Log.i("server", "Server started");

            Log.i("server", "Waiting for client...");
            socket = server.accept();
            Log.i("server", "Client connected");

            in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
            String line = "";
            while (!line.equals(".end")) {
                try {
                    line = in.readUTF();
                    Log.i("server", "Server received message: " + line);
                } catch (IOException i) {
                    Log.d("server", i.toString());
                }
            }

            Log.i("server", "Closing connection");
            socket.close();
            in.close();

        } catch (IOException i) {
            Log.d("server", i.toString());
        }
    }

    @Override
    public void run() {
        _Server(Server.PORT);
    }
}
