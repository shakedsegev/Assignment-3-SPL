
#include <string>
#include <map>
#include <sstream>
#include "StompProtocol.h"
#include "StompFrame.h"

StompProtocol::StompProtocol() : subscription_id_counter(1), receipt_id_counter(1), is_connected(false) {}
StompProtocol::~StompProtocol() {}

void StompProtocol::process_user_command(const std::string& input) {
        std::stringstream string_stream(input);
        std::string command;
        string_stream >> command;

        if (command == "login") {
            std::string host_port, username, passcode;
            string_stream >> host_port >> username >> passcode;
            frame_queue.push(StompFrame::create_connect_frame("stomp.cs.bgu.ac.il", username, passcode));
        }

        if (command == "join") {
            std::string game_name;
            string_stream >> game_name;
            int sub_id = subscription_id_counter++;
            int receipt_id = receipt_id_counter++;
            channel_to_sub_id[game_name] = sub_id;
            receipt_to_action[receipt_id] = "Joined channel " + game_name;
            frame_queue.push(StompFrame::create_subscribe_frame("/" + game_name, sub_id, receipt_id));
        }

        if (command == "exit") {
            std::string game_name;
            string_stream >> game_name;
            if (channel_to_sub_id.count(game_name)) {
                int sub_id = channel_to_sub_id[game_name];
                int receipt_id = receipt_id_counter++;
                receipt_to_action[receipt_id] = "Exited channel " + game_name;
                channel_to_sub_id.erase(game_name);
                frame_queue.push(StompFrame::create_unsubscribe_frame(sub_id, receipt_id));
            }
        }

        if (command == "logout") {
            int receipt_id = receipt_id_counter++;
            receipt_to_action[receipt_id] = "logout";
            frame_queue.push(StompFrame::create_disconnect_frame(receipt_id));
        }

        if (command == "report") {
            std::string file_path;
            string_stream >> file_path;

            // Use the provided parser from event.cpp
            names_and_events parsed = parseEventsFile(file_path);
            std::string game_name = parsed.team_a_name + "_" + parsed.team_b_name;

            for (const auto& event : parsed.events) {
                // Create a SEND frame for each event
                // And add them to the queue.
                StompFrame frame = StompFrame::create_send_frame("/" + game_name, event.to_string());
                frame_queue.push(frame);
            }
          
        }

        if (command == "summary") {
            std::string game_name, user, file_path;
            string_stream >> game_name >> user >> file_path;

            if (game_reports.count(game_name) && game_reports[game_name].count(user)) {
                std::vector<Event>& events = game_reports[game_name][user];
                std::stringstream ss;

                // Sort events by time if necessary
                // Format the output header
                ss << events[0].get_team_a_name() << " vs " << events[0].get_team_b_name() << "\n";
                ss << "Game stats:\nGeneral stats:\n";

                // Aggregate statistics and descriptions from all events
                for (const auto& event : events) {
                    ss << event.get_time() << " - " << event.get_name() << ":\n";
                    ss << event.get_discription() << "\n";
                }

                // Write to file
                std::ofstream out_file(file_path);
                out_file << ss.str();
                out_file.close();
            }
            
        }

    }

void StompProtocol::process_server_frame(const std::string& frame) {
    std::istringstream frame_stream(frame);

    StompFrame stomp_frame(frame);

    if(stomp_frame.command == "CONNECTED") {
        is_connected = true;
        std::cout << "Login successful" << std::endl;
    } else if (stomp_frame.command == "RECEIPT"){

        int receipt_id = std::stoi(stomp_frame.headers["receipt-id"]);

        if (receipt_to_action.count(receipt_id)) {
            std::string action = receipt_to_action[receipt_id];
            if (action == "logout") {
                is_connected = false;
                should_terminate_flag = true;
            } else {
                // Print the action associated with the receipt
                std::cout << action << std::endl;
            }
            receipt_to_action.erase(receipt_id);
        }
    } else if (stomp_frame.command == "MESSAGE") {
        std::string destination = stomp_frame.headers["destination"];
        std::string game_name = destination.substr(1); // Remove leading '/'
        Event event(stomp_frame.body);
        std::string reporter_name = event.get_user_name();
        game_reports[game_name][reporter_name].push_back(event);
        
    } else if (stomp_frame.command == "ERROR") {
        std::cout << stomp_frame.to_string() << std::endl;
        is_connected = false;
        should_terminate_flag = true;
    }
    
}

bool StompProtocol::has_frames_to_send() {
    return !frame_queue.empty();
}

StompFrame StompProtocol::get_next_frame() {
    StompFrame frame = frame_queue.front();
    frame_queue.pop();
    return frame;
}
