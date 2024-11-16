package com.example.androidserver;

import android.net.wifi.WifiManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.HashMap;

public class Server {
    public static final int PORT = 5000;

    protected ServerSocket  serverSocket = null;
    protected Handler handler;
    protected Listener listener;

    public Server(WifiManager wifiManager, int port) {

        Runtime.getRuntime().addShutdownHook(new Thread( () -> {
            try {
                serverSocket.close();
                handler.shutdown();
                listener.shutdown();
                Log.i("server", "Server shutdown hook");
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }));
        Log.i("server", String.format("(Server) Server registered shutdown hook", getLocalIpAddress(), PORT));

        try {
            serverSocket = new ServerSocket(port);
            Log.i("server", String.format("(Server) Server started at %s:%d", getLocalIpAddress(), PORT));

            handler = new Handler();
            Thread handlerThread = new Thread(handler);
            handlerThread.start();

            listener = new Listener(serverSocket, handler);
            Thread listenerThread = new Thread(listener);
            listenerThread.start();
        } catch (IOException i) {
            Log.d("server", i.toString());
        }
    }

    private class Listener implements Runnable {
        protected Socket socket;
        Listener(ServerSocket serverSocket, Handler handler) {
            Log.i("server", "Listener started");
        }

        public void shutdown() {}

        @Override
        public void run() {
            while (true) {
                Log.i("server", "Listener waiting for client...");
                try {
                    socket = serverSocket.accept();
                    handler.registerClient(socket);
                } catch (IOException e) {
                    Log.d("server", e.toString());
                }
            }
        }
    }

    private class Handler implements Runnable {
        public HashMap<Integer, Client> clients = new HashMap<>();
        Handler() {
            Log.i("server", "Handler started");
        }

        public void shutdown() {}

        public void registerClient(Socket socket) {
            Client client = new Client(socket);
            clients.put(client.id, client);
            Log.i("server", String.format("Handler registered client %d", client.id));
        }

        @Override
        public void run() {
            while (true) {
                // TODO may want to add a timeout here;
                //  if one client is broken and sending "infinite" messages,
                //  this for loop blocks on that client forever
                for (Client client : clients.values()) {
                    client.readMessageQueue();
                    Log.i("server", String.format("Handler processed client %d", client.id));
                }
            }
        }
    }

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


}
