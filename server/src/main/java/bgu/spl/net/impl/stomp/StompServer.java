package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: StompServer <port> <server_type>");
            return;
        }

        int port = Integer.parseInt(args[0]);
        String serverType = args[1];
        
        Connections<String> connections = new StompConnections();

        if (serverType.equals("tpc")) {
            Server.threadPerClient(
                port,
                StompMessagingProtocolImpl::new,
                StompEncoderDecoder::new,
                connections 
            ).serve();

        } else if (serverType.equals("reactor")) {
            Server.reactor(
                Runtime.getRuntime().availableProcessors(),
                port,
                StompMessagingProtocolImpl::new,
                StompEncoderDecoder::new,
                connections 
            ).serve();
        }
    }
}