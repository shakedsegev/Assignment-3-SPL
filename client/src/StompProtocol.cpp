
#include <string>
#include <map>
#include <sstream>
#include "StompProtocol.h"
#include "StompFrame.h"

StompProtocol::StompProtocol() : subscription_id_counter(1), receipt_id_counter(1), is_connected(false) {}
StompProtocol::~StompProtocol() {}

std::string StompProtocol::process_user_command(const std::string& input) {
        std::stringstream string_stream(input);
        std::string command;
        string_stream >> command;

        if (command == "login") {
            std::string host_port, username, passcode;
            string_stream >> host_port >> username >> passcode;
            return StompFrame::create_connect_frame("stomp.cs.bgu.ac.il", username, passcode);
        }

        if (command == "join") {
            std::string game_name;
            string_stream >> game_name;
            int sub_id = subscription_id_counter++;
            int receipt_id = receipt_id_counter++;
            channel_to_sub_id[game_name] = sub_id;
            receipt_to_action[receipt_id] = "Joined channel " + game_name;
            return StompFrame::create_subscribe_frame("/" + game_name, sub_id, receipt_id);
        }

        if (command == "exit") {
            std::string game_name;
            string_stream >> game_name;
            if (channel_to_sub_id.count(game_name)) {
                int sub_id = channel_to_sub_id[game_name];
                int receipt_id = receipt_id_counter++;
                receipt_to_action[receipt_id] = "Exited channel " + game_name;
                channel_to_sub_id.erase(game_name);
                return StompFrame::create_unsubscribe_frame(sub_id, receipt_id);
            }
        }

        if (command == "logout") {
            int receipt_id = receipt_id_counter++;
            receipt_to_action[receipt_id] = "logout";
            return StompFrame::create_disconnect_frame(receipt_id);
        }

        if (command == "report") {
            std::string file_path;
            string_stream >> file_path;
            // handle_report(file_path);
            return "";
        }

        if (command == "summary") {
            std::string game_name, user, file_path;
            string_stream >> game_name >> user >> file_path;
            // save_summary_to_file(game_name, user, file_path);
            return "";
        }

        return "";
    }

    void StompProtocol::parse_server_frame(const std::string& frame, std::string& command, std::map<std::string, std::string>& headers, std::string& body) {
        std::stringstream frame_stream(frame);
        std::string line;

        std::getline(frame_stream, line);
        
        // Remove carriage character if present (if the server is windows-based it might be added)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        command = line;

        while (std::getline(frame_stream, line) && line != "" && line != "\r") {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                headers[key] = value;
            }
        }

        std::string remaining;

        // Read body until null terminator
        std::getline(frame_stream, remaining, '\0');
        body = remaining;
    }

    std::map<std::string, Event> StompProtocol::parse_event_from_body(const std::string& body) {
        // TODO: implement parsing logic
        return std::map<std::string, Event>();
    }

    void StompProtocol::process_server_frame(const std::string& frame) {
        std::istringstream frame_stream(frame);
        std::string server_command;
        std::map<std::string, std::string> headers;
        std::string body;

        parse_server_frame(frame, server_command, headers, body);

        if(server_command == "CONNECTED") {
            is_connected = true;
            std::cout << "Login successful" << std::endl;
        } else if (server_command == "RECEIPT"){
            
            int receipt_id = std::stoi(headers["receipt-id"]);

            if (receipt_to_action.count(receipt_id)) {
                std::string action = receipt_to_action[receipt_id];
                if (action == "logout") {
                    is_connected = false;

                    // TODO: close connection
                } else {
                    // Print the action associated with the receipt
                    std::cout << action << std::endl;
                }
                receipt_to_action.erase(receipt_id);
            }
        } else if (server_command == "MESSAGE") {
            std::string destination = headers["destination"];
            std::string game_name = destination.substr(1); // Remove leading '/'
            std::map<std::string, Event> event_map = parse_event_from_body(body);
            if (!event_map.empty()) {
                auto it = event_map.begin();
                std::string reporter_name = it->first;
                Event event = it->second;
                // TODO finish message case 
            }
        } else if (server_command == "ERROR") {
            std::string error_message = body;
            std::cerr << "Error from server: " << error_message << std::endl;
        }
        
    }
