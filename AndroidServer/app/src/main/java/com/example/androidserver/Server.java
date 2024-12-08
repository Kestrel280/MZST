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

    public Course currentCourse;

    private ServerState state;
    private ActionHandler actionHandler;

    protected ServerSocket  serverSocket = null;
    public ClientHandler userHandler;
    public ClientHandler nodeHandler;
    protected ConnectionListener connectionListener;
    public ClientHandler transmitterHandler;
    private Thread userHandlerThread;
    private Thread nodeHandlerThread;
    private Thread transmitterHandlerThread;
    private Thread connectionListenerThread;

    /* ************************ */
    /*           Enums          */
    /* ************************ */

    public enum ServerState {
        IDLE,
        RUN,
        DEFINE,
        READY,
        FINISHED;
    }

    /* ************************ */
    /*   Classes/Constructors   */
    /* ************************ */

    public Server(WifiManager wifiManager, int port) {

        this.state = ServerState.IDLE;
        this.actionHandler = new ActionHandler(this);

        try {
            serverSocket = new ServerSocket(port);
            Log.i("server", String.format("(Server) Server started at %s:%d", getLocalIpAddress(), PORT));

            userHandler = new ClientHandler("userHandler");
            nodeHandler = new ClientHandler("nodeHandler");
            transmitterHandler = new ClientHandler("transmitterHandler");

            userHandlerThread = new Thread(userHandler);
            nodeHandlerThread = new Thread(nodeHandler);
            transmitterHandlerThread = new Thread(transmitterHandler);
            userHandlerThread.start();
            nodeHandlerThread.start();
            transmitterHandlerThread.start();

            connectionListener = new ConnectionListener(serverSocket, userHandler, nodeHandler, transmitterHandler);
            connectionListenerThread = new Thread(connectionListener);
            connectionListenerThread.start();
        } catch (IOException i) {
            Log.d("server", i.toString());
        }
    }

    private class ConnectionListener implements Runnable {
        public volatile boolean shutdown = false;
        protected Socket socket;
        ConnectionListener(ServerSocket serverSocket, ClientHandler userHandler, ClientHandler nodeHandler, ClientHandler transmitterHandler) {
            Log.i("server", "ConnectionListener started");
        }

        public void postShutdown() {
            shutdown = true;
        }

        @Override
        public void run() {
            do {
                Log.i("server", "ConnectionListener waiting for clients...");
                try {
                    ClientMessage registrationMessage = null;
                    socket = serverSocket.accept();
                    Client client = new Client(socket);
                    while ((registrationMessage == null) && !shutdown) {
                        registrationMessage = client.readMessage();
                    }
                    client.id = registrationMessage.id;
                    Log.i("debug", String.format("connected client id is %d", client.id));

                    switch (registrationMessage.code) {
                        case ClientMessage.MTYPE_REGISTER_USER: client.setType(Client.USER); userHandler.registerClient(client); break;
                        case ClientMessage.MTYPE_REGISTER_NODE: client.setType(Client.NODE); nodeHandler.registerClient(client); break;
                        case ClientMessage.MTYPE_REGISTER_TRANSMITTER: client.setType(Client.TRANSMITTER); transmitterHandler.registerClient(client); break;
                        default: {
                            Log.i("server", "Received invalid registration message type %d; rejecting connection");
                            socket.close();
                        }
                    }
                } catch (IOException e) {
                    Log.d("server", e.toString());
                }
            } while (!shutdown);
            try {
                serverSocket.close();
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    public class ClientHandler implements Runnable {
        private volatile boolean shutdown = false;
        public String name;
        public HashMap<Integer, Client> clients = new HashMap<>();
        Queue<Pair<Integer, ServerMessage>> outboundMessageQueue = new ArrayDeque<>();
        Queue<ServerMessage> outboundBroadcastQueue = new ArrayDeque<>();
        Queue<Client> newClientsQueue = new ArrayDeque<>();

        ClientHandler(String handlerName) {
            Log.i("server", "ClientHandler " + handlerName + " started");
            this.name = handlerName;
        }

        public void postShutdown() {
            shutdown = true;
        }

        public void registerClient(Client client) {
            newClientsQueue.add(client);
        }

        public void postBroadcast(ServerMessage msg) {
            outboundBroadcastQueue.add(msg);
        }

        public void postMsg(int clientId, ServerMessage msg) {
            outboundMessageQueue.add(new Pair<>(clientId, msg));
        }

        @Override
        public void run() {
            ServerMessage response;
            List<ClientMessage> inboundMessages;
            do {
                // TODO may want to add timeouts here;

                while (!newClientsQueue.isEmpty()) {
                    Client newClient = newClientsQueue.remove();
                    clients.put(newClient.id, newClient); // TODO this will overwrite any client with same id. may want to add a warning
                    MainActivity.debugMsgToAppView(String.format("%s registered client id %d", this.name, newClient.id));
                    Log.i("server", String.format("%s registered client %d", this.name, newClient.id));
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
            } while (!shutdown);
        }
    }

    /* ************************ */
    /*     Message  handling    */
    /* ************************ */

    private ServerMessage processMessage(ClientMessage cMsg) {
        ServerMessage response = new ServerMessage("ACK");

        switch (cMsg.code) {
            case ClientMessage.MTYPE_TRIGGER: {
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

    /* ************************ */
    /*     Action Processing    */
    /* ************************ */

    public ServerState getState() {
        return state;
    }

    public void setState(ServerState newState) {
        Log.i("server", "Server state set to " + newState.toString());
        state = newState;
    }

    public String sendAction(ServerAction action) {
        Log.i("server", "Server received action of type " + action.type.toString());
        actionHandler.processAction(action);
        return "";
    }

    /* ************************ */
    /*     Course Management    */
    /* ************************ */

    public void createNewCourse() {
        this.currentCourse = new Course();
    }

    public void saveCurrentCourse() {
        this.currentCourse.finishEditing();
        // TODO prompt for name
        // TODO save to database
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

    public void shutdown() {
        try {
            Log.i("server", "Server shutting down; sending SHUTDOWN message");
            nodeHandler.postBroadcast(new ServerMessage("SHUTDOWN"));
            userHandler.postBroadcast(new ServerMessage("SHUTDOWN"));
            transmitterHandler.postBroadcast(new ServerMessage("SHUTDOWN"));
            connectionListener.postShutdown();
            userHandler.postShutdown();
            nodeHandler.postShutdown();
            transmitterHandler.postShutdown();

            //connectionListenerThread.join();
            userHandlerThread.join();
            nodeHandlerThread.join();
            transmitterHandlerThread.join();

            Log.i("server", " --- Server successfully shut down ---");
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

}
