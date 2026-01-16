package bgu.spl.net.impl.stomp;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.impl.stomp.StompFrame;

public class StompConnections<T> implements Connections<T> {

    // Maps connectionId -> connectionHandler of the client with that ID
    private final ConcurrentHashMap<Integer, ConnectionHandler<T>> activeConnections = new ConcurrentHashMap<>();

    // Maps channelName -> (connectionId -> clientSubscriptionId)
    private final ConcurrentHashMap<String, ConcurrentHashMap<Integer, String>> channels = new ConcurrentHashMap<>();
    
    // Counter for connection IDs
    private int connectionIdCounter = 0;

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = activeConnections.get(connectionId);
        if (handler != null) {
            handler.send(msg); 
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {

        ConcurrentHashMap<Integer, String> channelSubscribers = channels.get(channel);

        // Channel does not exist
        if(channelSubscribers == null) { return;}

        for (Map.Entry<Integer, String> entry : channelSubscribers.entrySet()) {
            Integer connectionId = entry.getKey();
            String subscriptionId = entry.getValue();


            
        }

    }

    @Override
    public void disconnect(int connectionId) {
        activeConnections.remove(connectionId);
        // Also remove this ID from all topics in 'channels'
    }

    // Helper to add new connections (called by Server)
    public void addConnection(int id, ConnectionHandler<T> handler) {
        activeConnections.put(id, handler);
    }
}