#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

int main(int argc, char *argv[]) {
    std::string pending_line;
    // The outer loop allows the app to stay open even if we logout/disconnect
    while (true) {
		
        StompProtocol protocol;
        ConnectionHandler* connection = nullptr;
        std::thread networkThread;

        // Active session of a user
        while (!protocol.should_terminate()) {
            std::string line;
            if (!pending_line.empty()) {
                line = pending_line;
                pending_line.clear();
            } else {
                if (!std::getline(std::cin, line)) break;
            
                // Check if during the wait for user input, the network thread decided to terminate
                if (protocol.should_terminate()) {
                    pending_line = line;
                    break;
                }
            }

            ConnectionInfo info = protocol.process_user_command(line);

			// If user did login command, connect to server
            if (info.should_connect) {

				if (connection) {
                    std::cout << "The client is already logged in, log out before trying again" << std::endl;
                    continue;
                }

                connection = new ConnectionHandler(info.host, info.port);
                if (!connection->connect()) {
                    std::cout << "Could not connect to server" << std::endl;
                    delete connection; 
					connection = nullptr;
                    continue;
                }

                networkThread = std::thread([connection, &protocol]() {
                    while (!protocol.should_terminate()) {
                        std::string frame;
                        if (connection->getFrameAscii(frame, '\0')) {
                            protocol.process_server_frame(frame);
                        } else {
                            break; // Connection closed
                        }
                    }
                });
            }

            if (connection) {
                while (protocol.has_frames_to_send()) {
                    StompFrame frame = protocol.get_next_frame();
                
                    if (frame.command != "EMPTY" && !connection->sendFrameAscii(frame.to_string(), '\0')) {
                        std::cerr << "Failed to send frame" << std::endl;
                        break;
                    }
                }
            }
        }

        // Cleanup before potentially logging in again
        if (networkThread.joinable()) networkThread.join();
        if (connection) {
            delete connection;
            connection = nullptr;
        }
        
        if (std::cin.eof()) break;

        protocol.reset_termination();
    }
    return 0;
}