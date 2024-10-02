package com.example.androidserver;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.Enumeration;

public class Server {
    public static final int PORT = 5000;

    private Socket          socket = null;
    private ServerSocket    server = null;
    private BufferedReader in     = null;

    public static String getLocalIpAddress() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress() && inetAddress instanceof Inet4Address) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        } catch (SocketException ex) {
            ex.printStackTrace();
        }
        return null;
    }

    public Server(WifiManager wifiManager, int port) {
        try {
            Thread.sleep(500);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
        try {
            server = new ServerSocket(port);
            Log.i("server", String.format("Server started at %s:%d", getLocalIpAddress(), PORT));

            Log.i("server", "Waiting for client...");
            socket = server.accept();
            Log.i("server", "Client connected");

            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            String line = "";
            Log.i("server", "Beginning message loop");
            while ((line = in.readLine()) != null) {
                Log.i("server", "Server received message: " + line);
                
            }

            Log.i("server", "Closing connection");
            socket.close();
            in.close();

        } catch (IOException i) {
            Log.d("server", i.toString());
        }
    }
}
