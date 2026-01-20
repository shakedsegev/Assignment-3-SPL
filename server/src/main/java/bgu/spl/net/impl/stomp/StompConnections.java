package bgu.spl.net.impl.stomp;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import bgu.spl.net.Mysrv.MyConnectionHandler;
import bgu.spl.net.Mysrv.MyConnections;

public class StompConnections implements MyConnections<String> {

    // Maps connectionId -> connectionHandler of the client with that ID
    private final ConcurrentHashMap<Integer, MyConnectionHandler<String>> activeConnections = new ConcurrentHashMap<>();

    // Maps channelName -> (connectionId -> clientSubscriptionId)
    private final ConcurrentHashMap<String, ConcurrentHashMap<Integer, String>> channels = new ConcurrentHashMap<>();
    

    // MessageId incremented everytime a Message frame is sent
    private AtomicInteger messageId = new AtomicInteger(0);

    @Override
    public boolean send(int connectionId, String msg) {
        MyConnectionHandler<String> handler = activeConnections.get(connectionId);
        if (handler != null) {
            handler.send(msg); 
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, String msg) {
        ConcurrentHashMap<Integer, String> channelSubscribers = channels.get(channel);

        // Channel does not exist
        if(channelSubscribers == null) { return;}

        // Increment messageId
        int currentMessageId = messageId.incrementAndGet();

        for (Map.Entry<Integer, String> entry : channelSubscribers.entrySet()) {
            Integer connectionId = entry.getKey();
            String subscriptionId = entry.getValue();

            StompFrame frame = StompFrame.createMessageFrame(subscriptionId, String.valueOf(currentMessageId), channel, msg);
            
            send(connectionId, frame.toString());
        }

    }

    @Override
    public void disconnect(int connectionId) {
        
        activeConnections.remove(connectionId);
        
        // Remove connectionId (the user) from every channel he is subscribed to (effectivly unsubscribing him from every channel)
        for (Map.Entry<String, ConcurrentHashMap<Integer, String>> entry : channels.entrySet()) {
            entry.getValue().remove(connectionId);
        }
    }

    // Add a new connection
    public void addConnection(int id, MyConnectionHandler<String> handler) {
        activeConnections.put(id, handler);
    }

    // Subscribe user to channel
    public void subscribe(String channel, int connectionId, String subscriptionId) {
        
        // Create new topic if it does not exist (check and put in one operation)
        channels.putIfAbsent(channel, new ConcurrentHashMap<>());

        // We never delete channels so this get after a put is safe.
        channels.get(channel).put(connectionId, subscriptionId);
    }


    // Unsubscribe user from channel
    public void unsubscribe(String channel, int connectionId) {
        Map<Integer, String> subscribers = channels.get(channel);
        if (subscribers != null) {
            subscribers.remove(connectionId);
        }
    }
    
    // Checks if a user is subscribed to a channel
    public boolean isSubscribed(String channel, int connectionId) {
        Map<Integer, String> subscribers = channels.get(channel);
        return subscribers != null && subscribers.containsKey(connectionId);
    }
}