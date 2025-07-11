package com.example.androidserver;

import android.net.wifi.WifiManager;
import android.util.Log;
import android.util.Pair;

import com.example.androidserver.ServerAction.ActionType;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.InputStream;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

public class Server {
    public static final int PORT = 5000;

    public Course currentCourse;
    public ArrayList<Integer> currentCourseRemainingNodes;

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
    private JSONObject clientStates;

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

    public Server(WifiManager wifiManager, int port, JSONObject clientStates) {

        this.setState(ServerState.IDLE);
        this.actionHandler = new ActionHandler(this);

        try {
            serverSocket = new ServerSocket(port);

            this.clientStates = clientStates;

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
            Log.i("server", String.format("(Server) Server started at %s:%d", getLocalIpAddress(), PORT));
        } catch (IOException i) {
            Log.d("server", i.toString());
        }
    }

    // Helper class...
    // If the server wants to block until receiving a specific message type from a specific client,
    //  it can create a Promise and register it using clientHandler.postPromise(promise).
    //  Then, it can choose to block until that Promise is resolved using clientHandler.awaitPromise(promise).
    // The Handler maintains an internal linked list of Promises.
    //  Whenever the handler receives a msg, it will scan the linked list for any Promises
    //  which match the received message. If the message matches, the message is stored
    //  in the Promise's 'msg' field, and 'delivered' is set to true.
    private class Promise {
        public int clientId;
        public int mType;
        public volatile boolean resolved;
        public ClientMessage msg;
        Promise(int clientId, int mType) {
            this.resolved = false;
            this.clientId = clientId;
            this.mType = mType;
            this.msg = null;
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

                    switch (registrationMessage.code) {
                        case ClientMessage.MTYPE_REGISTER_USER: client.setType(Client.USER); userHandler.registerClient(client, registrationMessage.data); break;
                        case ClientMessage.MTYPE_REGISTER_NODE: client.setType(Client.NODE); nodeHandler.registerClient(client, registrationMessage.data); break;
                        case ClientMessage.MTYPE_REGISTER_TRANSMITTER: client.setType(Client.TRANSMITTER); transmitterHandler.registerClient(client, registrationMessage.data); break;
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
        LinkedList<Promise> promises;

        ClientHandler(String handlerName) {
            Log.i("server", "ClientHandler " + handlerName + " started");
            promises = new LinkedList<>();
            this.name = handlerName;
        }

        public void postShutdown() {
            shutdown = true;
        }

        public void registerClient(Client client, int data) {
            Log.i("server", String.format("%s registering client %d of type %d with data %d", this.name, client.id, client.type, data));
            MainActivity.debugMsgToAppView(String.format("%s registering client %d of type %d w/ data = %d", this.name, client.id, client.type, data), "debug");
            newClientsQueue.add(client);

            // Send client STATE_DEFINE messages for each state we may use
            for (Iterator<String> it = clientStates.keys(); it.hasNext(); ) {
                String key = it.next();
                try {
                    JSONObject stateEntry = clientStates.getJSONObject(key);
                    int stateId = (int) stateEntry.get("id");
                    JSONArray colorNeutral = stateEntry.getJSONArray("color_neutral");
                    JSONArray colorTouched = stateEntry.getJSONArray("color_touched");
                    postMsg(client.id, (new ServerMessage(String.format("REQ_ACK STATE_DEFINE %d %d %d %d %d %d %d", stateId, (int) colorNeutral.get(0), (int) colorNeutral.get(1), (int) colorNeutral.get(2), (int) colorTouched.get(0), (int) colorTouched.get(1), (int) colorTouched.get(2)))));
                    //Promise clientAck = new Promise(client.id, ClientMessage.MTYPE_ACK);
                    //postPromise(clientAck);
                    //awaitPromise(clientAck);
                } catch (JSONException e) {
                    throw new RuntimeException(e);
                }
            }
        }

        public void postBroadcast(ServerMessage msg) {
            outboundBroadcastQueue.add(msg);
        }

        public void postMsg(int clientId, ServerMessage msg) {
            outboundMessageQueue.add(new Pair<>(clientId, msg));
        }

        public void postPromise(Promise promise) {
            this.promises.add(promise);
        }

        // Blocks the caller until the handler receives a message from a specific client of a specific type.
        // See the comments on Promise class for details.
        public void awaitPromise(Promise promise) {
            while (!promise.resolved) {
                Thread.currentThread().yield();
            }
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
                        for (Promise promise : promises) {
                            if ((inboundMessage.id == promise.clientId) && (inboundMessage.code == promise.mType)) {
                                promise.msg = inboundMessage;
                                promise.resolved = true;
                                promises.remove(promise);
                            }
                        }
                        if (response != null) client.sendMessage(response);
                    }
                }
            } while (!shutdown);
        }
    }

    /* ************************ */
    /*     Message  handling    */
    /* ************************ */

    private ServerMessage processMessage(ClientMessage cMsg) {
        ServerMessage response = null;
        switch (cMsg.code) {
            case ClientMessage.MTYPE_TRIGGER: {

                // TODO Placeholder for later.
                //      If server is READY,
                //      and a TRIGGER message is received,
                //      and it's from the first node in the current course,
                //      interpret it as a START message.
                //      This should be integrated into the sensor packages later, such that they send dedicated START messages.
                ActionType type = (state == ServerState.READY && cMsg.id == currentCourse.nodeSequence.get(0) ? ActionType.START : ActionType.TRIGGER);

                response = actionHandler.processAction(
                        //new ServerAction(ActionType.TRIGGER).
                        new ServerAction(type)
                                .setClientId(cMsg.id)
                                .setData(cMsg.data));
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
        MainActivity.debugMsgToAppView(newState.toString(), "state");
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
        nodeHandler.postBroadcast(new ServerMessage("SET_STATE DEFINE_NotInCourse"));
        this.currentCourse = new Course();
    }

    public void saveCurrentCourse() {
        this.currentCourse.finishEditing();
        // TODO prompt for name
        // TODO save to database
    }

    public void addCourseNode(int nodeId) {
        // Check to make sure there is a previously-touched node. If so, set its state to DEFINE_InCourse
        if (!currentCourse.nodeSequence.isEmpty()) {

            // If nodeId is already the last node in the course, send it an UNTOUCH message and then short-circuit
            if (currentCourse.nodeSequence.get(currentCourse.nodeSequence.size() - 1) == nodeId) {
                nodeHandler.postMsg(nodeId, new ServerMessage("UNTOUCH"));
                return;
            }

            nodeHandler.postMsg(
                    currentCourse.nodeSequence.get(currentCourse.nodeSequence.size() - 1),
                    new ServerMessage("SET_STATE DEFINE_InCourse"));
        }

        // Now set the state of the newly touched node to DEFINE_SelectedNode
        nodeHandler.postMsg(nodeId, new ServerMessage("SET_STATE DEFINE_SelectedNode"));

        // Finally, add the node to the course
        currentCourse.addNode(nodeId);
    }

    public void prepCurrentCourse() {
        Promise promise;
        transmitterHandler.postBroadcast(new ServerMessage("REQ_ACK TRANSMIT"));

        // Set the state of each node, one by one
        // Wait for an ACK between each message
        // The FIRST course node (currentCourse.nodeSequence.first()) gets "SET_STATE READYRUN_StartNode"
        // Rest of the course nodes get "SET_STATE READYRUN_NoTriggersDone"
        // Everything else gets "SET_STATE READYRUN_NotPartOfCourse"
        for (Client client : nodeHandler.clients.values()) {
            Log.i("debug", "prepping node " + client.id);
            promise = new Promise(client.id, ClientMessage.MTYPE_ACK);
            nodeHandler.postPromise(promise);

            if (currentCourse.nodeSequence.get(0) == client.id) {
                nodeHandler.postMsg(client.id, new ServerMessage("REQ_ACK SET_STATE READYRUN_StartNode"));
            } else if (currentCourse.nodeSequence.contains(client.id)) {
                nodeHandler.postMsg(client.id, new ServerMessage("REQ_ACK SET_STATE READYRUN_NoTriggersDone"));
            } else {
                nodeHandler.postMsg(client.id, new ServerMessage("REQ_ACK SET_STATE READYRUN_NotPartOfCourse"));
            }

            nodeHandler.awaitPromise(promise);
        }

        currentCourseRemainingNodes = new ArrayList<>(currentCourse.nodeSequence);
    }

    public boolean advanceCourse(int nodeId, long timestamp) {
        // Tries to update the state of the current course being run (currentCourseRemainingNodes)
        // Returns true if the course is finished (whether or not it's a successful run), false otherwise

        // If the correct node was hit, remove it from currentCourseRemainingNodes
        if (currentCourseRemainingNodes.get(0) == nodeId) {
            currentCourseRemainingNodes.remove(0);

            // Check for win
            if (currentCourseRemainingNodes.isEmpty()) {
                nodeHandler.postBroadcast(new ServerMessage("REQ_ACK SET_STATE FINISHED_SuccessfulRun"));
                return true;
            }

            // No win; update status of this node and continue
            if (currentCourseRemainingNodes.contains(nodeId)) {
                nodeHandler.postMsg(nodeId, new ServerMessage("REQ_ACK SET_STATE READYRUN_SomeTriggersDone"));
            } else {
                nodeHandler.postMsg(nodeId, new ServerMessage("REQ_ACK SET_STATE READYRUN_AllTriggersDone"));
            }
            return false;
        }
        // If the INCORRECT node was hit, untrigger it
        else {
            //nodeHandler.postBroadcast(new ServerMessage("REQ_ACK SET_STATE FINISHED_UnsuccessfulRun"));
            nodeHandler.postBroadcast(new ServerMessage("UNTOUCH"));
            return false;
        }
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
