package bgu.spl.net.Mysrv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.StompMessagingProtocol;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.function.Supplier;

public abstract class MyBaseServer<T> implements MyStompServer<T> {

    private final int port;
    private final Supplier<StompMessagingProtocol<T>> protocolFactory;
    private final Supplier<MessageEncoderDecoder<T>> encdecFactory;
    private ServerSocket sock;

    private MyConnections<T> connections; 
    private int connectionIdCounter = 0;

    public MyBaseServer(
            int port,
            Supplier<StompMessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> encdecFactory,
            MyConnections<T> connections) {

        this.port = port;
        this.protocolFactory = protocolFactory;
        this.encdecFactory = encdecFactory;
        this.connections = connections;
		this.sock = null;
    }

    @Override
    public void serve() {

        try (ServerSocket serverSock = new ServerSocket(port)) {
			System.out.println("Server started");

            this.sock = serverSock; //just to be able to close

            while (!Thread.currentThread().isInterrupted()) {

                Socket clientSock = serverSock.accept();

                StompMessagingProtocol<T> protocol = protocolFactory.get();
                int connectionId = connectionIdCounter++;

                protocol.start(connectionId, connections);

                MyBlockingConnectionHandler<T> handler = new MyBlockingConnectionHandler<>(
                        clientSock,
                        encdecFactory.get(),
                        protocol);

                connections.addConnection(connectionId, handler);

                execute(handler);
            }
        } catch (IOException ex) {
        }

        System.out.println("server closed!!!");
    }

    @Override
    public void close() throws IOException {
		if (sock != null)
			sock.close();
    }

    protected abstract void execute(MyBlockingConnectionHandler<T>  handler);

}
