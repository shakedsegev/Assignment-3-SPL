package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.Map;

import bgu.spl.net.Mysrv.MyConnections;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.data.Database;
import bgu.spl.net.impl.data.LoginStatus;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String>{
    
    private boolean shouldTerminate = false;
    private boolean isLoggedIn = false;
    private int connectionId;
    private MyConnections<String> connectionsInstance;

    // Key: Subscription ID, Value: Channel Name
    private Map<String, String> activeSubscriptions = new HashMap<>();
    private String current_user;
    
    /**
	 * Used to initiate the current client protocol with it's personal connection ID and the connections implementation
	**/
    @Override
    public void start(int connectionId, MyConnections<String> connections) {
        this.connectionId = connectionId;
        this.connectionsInstance = connections;
    }
    
    @Override
    public void process(String message) {

        StompFrame frame;

        try {
            frame = StompFrame.fromString(message);
        } catch (Exception e) {
            sendError("Invalid Frame", "Could not parse frame", null);
            return;
        }

        String command = frame.getCommand();

        switch (command) {
            case "CONNECT":
                handleConnect(frame);
                break;
            case "SUBSCRIBE":
                handleSubscribe(frame);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(frame);
                break;
            case "SEND":
                handleSend(frame);
                break;
            case "DISCONNECT":
                handleDisconnect(frame);
                break;
            default:
                sendError("Unknown Command", command, null);
        }
    }
	
	/**
     * @return true if the connection should be terminated
     */
    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private void handleConnect(StompFrame frame) {
        
        String login = frame.getHeaders().get("login");
        String passcode = frame.getHeaders().get("passcode");
        String receipt = frame.getHeaders().get("receipt");

        LoginStatus status = Database.getInstance().login(connectionId, login, passcode);

        switch (status) {
            case LOGGED_IN_SUCCESSFULLY:
            case ADDED_NEW_USER:
                // Set protocol state
                this.isLoggedIn = true;
                this.current_user = login;
                
                connectionsInstance.send(connectionId, StompFrame.createConnectedFrame().toString());
                
                if (receipt != null) sendReceipt(receipt);
                break;

            case ALREADY_LOGGED_IN:
                sendError("User already logged in", "User '" + login + "' is already active from another client", receipt);
                break;

            case WRONG_PASSWORD:
                sendError("Wrong password", "Password mismatch for user '" + login + "'", receipt);
                break;

            case CLIENT_ALREADY_CONNECTED:
                sendError("Client already connected", "This socket is already associated with an active user", receipt);
                break;

            default:
                sendError("Login failed", "An unknown error occurred during authentication", receipt);
        }
    }

    private void handleSubscribe(StompFrame frame) {
        
        if (!isLoggedIn) {
            sendError("User not logged in", "Please log in first", null);
            return;
        }
        
        String topic = frame.getHeaders().get("destination");
        String id = frame.getHeaders().get("id");
        String receipt = frame.getHeaders().get("receipt");
        
        if (topic == null || id == null) {
            sendError("Missing headers", "destination or id headers missing..." , receipt);
            return;
        }
        
        ((StompConnections) connectionsInstance).subscribe(topic, connectionId, id);
        activeSubscriptions.put(id, topic);
        
        // Handle Receipt
        if (receipt != null) sendReceipt(receipt);
    }
    
    private void handleUnsubscribe(StompFrame frame) {
        
        if (!isLoggedIn) {
            sendError("User not logged in", "Please log in first", null);
            return;
        }
        
        String id = frame.getHeaders().get("id");
        String receipt = frame.getHeaders().get("receipt");

        if (id == null) {
             sendError("Missing headers", "id header is missing...", receipt);
             return;
        }

        String topic = activeSubscriptions.get(id);

        // TODO: Do we need to handle someone unsubscribing from a topic he is no subscribed to?
        if (topic == null) {
            sendError("Not Subscribed", "Cannot unsubscribe to channel you are not subscribed to", receipt);
            return;
        }

        ((StompConnections) connectionsInstance).unsubscribe(id, connectionId);
        activeSubscriptions.remove(id);
        
        if (receipt != null) sendReceipt(receipt);
    }

    private void handleSend(StompFrame frame) {
        
        if (!isLoggedIn) {
            sendError("User not logged in", "Please log in first", null);
            return;
        }
        
        String topic = frame.getHeaders().get("destination");
        String receipt = frame.getHeaders().get("receipt");

        if (topic == null) {
            sendError("Missing headers", "destination header is missing", receipt);
            return;
        }

        // Verify user is subscribed
        if (!((StompConnections)connectionsInstance).isSubscribed(topic, connectionId)) {
            sendError("Not Subscribed", "Cannot send to channel you are not subscribed to", receipt);
            return;
        }
        String event = frame.getBody();

        if (event.contains("event name:")) {
            Database.getInstance().trackFileUpload(current_user, "Game_Reports", topic);
        }

        // Send event string to channel (will be wrapped as a frame in send)
        connectionsInstance.send(topic, event);
    }

    private void handleDisconnect(StompFrame frame) {
        
        if (!isLoggedIn) {
            sendError("User not logged in", "Please log in first", null);
            return;
        }
        
        String receipt = frame.getHeaders().get("receipt");
        
        if (receipt != null) sendReceipt(receipt);
        
        Database.getInstance().logout(connectionId);

        shouldTerminate = true;

        connectionsInstance.disconnect(connectionId);

    }
    
    private void sendReceipt(String receiptId) {

        StompFrame receipt = StompFrame.createReceiptFrame(receiptId);

        connectionsInstance.send(connectionId, receipt.toString());
    }

    private void sendError(String msg, String description, String receiptId) {

        StompFrame errorFrame = StompFrame.createErrorFrame(msg, description, receiptId);

        connectionsInstance.send(connectionId, errorFrame.toString());
        shouldTerminate = true;
        connectionsInstance.disconnect(connectionId);
    }
}
