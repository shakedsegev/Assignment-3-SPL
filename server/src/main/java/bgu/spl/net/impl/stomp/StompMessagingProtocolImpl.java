package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.Map;

import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.stomp.StompConnections;
import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String>{
    
    private boolean shouldTerminate = false;
    private boolean isLoggedIn = false;
    private int connectionId;
    private Connections<String> connectionsInstance;

    // Key: Subscription ID, Value: Channel Name
    private Map<String, String> activeSubscriptions = new HashMap<>();
    
    /**
	 * Used to initiate the current client protocol with it's personal connection ID and the connections implementation
	**/
    @Override
    public void start(int connectionId, Connections<String> connections) {
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
        
        if (isLoggedIn) {
            sendError("User already logged in", "Logout first before trying to log in again", null);
            return;
        }

        String login = frame.getHeaders().get("login");
        String passcode = frame.getHeaders().get("passcode");
        String receipt = frame.getHeaders().get("receipt");
        
        if (login == null || passcode == null) {
            sendError("Missing headers", "login and/or passcode header missing...", receipt);
            return;
        }
        
        // TODO: Add User/Password check logic here later
        // Check in DB
        // Check password

        isLoggedIn = true;
        connectionsInstance.send(connectionId, StompFrame.createConnectedFrame().toString());

        if (receipt != null) sendReceipt(receipt);
    }

    private void handleSubscribe(StompFrame frame) {
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
        
        if (receipt != null) sendReceipt(receipt);
    }

    private void handleSend(StompFrame frame) {
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

        // Send event string to channel (will be wrapped as a frame in send)
        connectionsInstance.send(topic, event);
    }

    private void handleDisconnect(StompFrame frame) {
        String receipt = frame.getHeaders().get("receipt");
        
        shouldTerminate = true;
        connectionsInstance.disconnect(connectionId);

        if (receipt != null) sendReceipt(receipt);
    }
    
    private void sendReceipt(String receiptId) {

        StompFrame receipt = StompFrame.createReceiptFrame(receiptId);

        connectionsInstance.send(connectionId, receipt.toString());
    }

    private void sendError(String msg, String description, String receitId) {

        StompFrame errorFrame = StompFrame.createErrorFrame(msg, description, receitId);

        connectionsInstance.send(connectionId, errorFrame.toString());
        shouldTerminate = true;
        connectionsInstance.disconnect(connectionId);
    }
}
