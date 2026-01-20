package bgu.spl.net.Mysrv;

public interface MyConnections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    void addConnection(int id, MyConnectionHandler<T> handler);
}
