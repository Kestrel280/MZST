package com.example.androidserver;

import android.net.wifi.WifiManager;
import android.util.Log;
import android.util.Pair;

import com.example.androidserver.ServerAction.ActionType;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.ArrayDeque;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Queue;

public class Server {
    public static final int PORT = 5000;

    private ServerState state;
    private ActionHandler actionHandler;

    protected ServerSocket  serverSocket = null;
    protected ClientHandler clientHandler;
    protected ConnectionListener connectionListener;

    public ServerState getState() {
        return state;
    }

    public enum ServerState {
        IDLE,
        RUN,
        DEFINE,
        READY,
        FINISHED;
    }

    public Server(WifiManager wifiManager, int port) {

        Runtime.getRuntime().addShutdownHook(new Thread( () -> {
            try {
                serverSocket.close();
                clientHandler.shutdown();
                connectionListener.shutdown();
                Log.i("server", "Server shutdown hook");
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }));
        Log.i("server", String.format("(Server) Server registered shutdown hook", getLocalIpAddress(), PORT));

        this.state = ServerState.IDLE;
        this.actionHandler = new ActionHandler(this);

        try {
            serverSocket = new ServerSocket(port);
            Log.i("server", String.format("(Server) Server started at %s:%d", getLocalIpAddress(), PORT));

            clientHandler = new ClientHandler();
            Thread handlerThread = new Thread(clientHandler);
            handlerThread.start();

            connectionListener = new ConnectionListener(serverSocket, clientHandler);
            Thread listenerThread = new Thread(connectionListener);
            listenerThread.start();
        } catch (IOException i) {
            Log.d("server", i.toString());
        }
    }

    private class ConnectionListener implements Runnable {
        protected Socket socket;
        ConnectionListener(ServerSocket serverSocket, ClientHandler clientHandler) {
            Log.i("server", "Listener started");
        }

        public void shutdown() {}

        @Override
        public void run() {
            while (true) {
                Log.i("server", "Listener waiting for client...");
                try {
                    socket = serverSocket.accept();
                    Client client = new Client(socket);
                    clientHandler.registerClient(client);
                } catch (IOException e) {
                    Log.d("server", e.toString());
                }
            }
        }
    }

    public class ClientHandler implements Runnable {
        public HashMap<Integer, Client> clients = new HashMap<>();
        Queue<Pair<Integer, ServerMessage>> outboundMessageQueue = new ArrayDeque<>();
        Queue<ServerMessage> outboundBroadcastQueue = new ArrayDeque<>();
        Queue<Pair<Integer, Client>> newClientsQueue = new ArrayDeque<>();
        ClientHandler() {
            Log.i("server", "Handler started");
        }

        public void shutdown() {}

        public void registerClient(Client client) {
            newClientsQueue.add(new Pair<>(client.id, client));
        }

        public void postBroadcast(ServerMessage msg) {
            outboundBroadcastQueue.add(msg);
        }

        public void postMsg(int nodeId, ServerMessage msg) {
            outboundMessageQueue.add(new Pair<>(nodeId, msg));
        }

        @Override
        public void run() {
            ServerMessage response;
            List<ClientMessage> inboundMessages;
            while (true) {
                // TODO may want to add timeouts here;

                while (!newClientsQueue.isEmpty()) {
                    Pair<Integer, Client> p = newClientsQueue.remove();
                    clients.put(p.first, p.second);
                    MainActivity.debugMsgToAppView(String.format("Registered client %d", p.first));
                    Log.i("server", String.format("Handler registered client %d", p.first));
                }

                while (!outboundMessageQueue.isEmpty()) {
                    Pair<Integer, ServerMessage> p = outboundMessageQueue.remove();
                    Client client = clients.get(p.first);
                    if (client != null) {
                        client.sendMessage(p.second);
                    }
                }

                while (!outboundBroadcastQueue.isEmpty()) {
                    ServerMessage msg = outboundBroadcastQueue.remove();
                    for (Client client : clients.values()) {
                        client.sendMessage(msg);
                    }
                }

                for (Client client : clients.values()) {
                    inboundMessages = client.readMessageQueue();
                    for (ClientMessage inboundMessage : inboundMessages) {
                        response = processMessage(inboundMessage);
                        client.sendMessage(response);
                    }
                }
            }
        }
    }

    private ServerMessage processMessage(ClientMessage cMsg) {
        ServerMessage response = new ServerMessage("ACK");

        switch (cMsg.type) {
            case ClientMessage.TRIGGERED: {
                actionHandler.processAction(
                        new ServerAction(ActionType.TRIGGER)
                                .setClientId(cMsg.id)
                                .setTimestamp(cMsg.timestamp));
                response.text = "RESET";
                break;
            }
            default: break;
        }
        return response;
    }

    public String sendAction(ServerAction action) {
        Log.i("server", "Server received action of type " + action.type.toString());
        actionHandler.processAction(action);
        return "";
    }

    /* *****************************
        Utilities
     * *****************************/

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
