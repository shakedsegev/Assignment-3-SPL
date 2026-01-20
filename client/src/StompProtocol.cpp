
#include <string>
#include <map>
#include <sstream>
#include "StompProtocol.h"
#include <fstream>

StompProtocol::StompProtocol() : current_user(""), key_mutex(), frame_queue(), subscription_id_counter(1), receipt_id_counter(1), is_connected(false), should_terminate_flag(false), receipt_to_action(), channel_to_sub_id(), game_reports() {}
StompProtocol::~StompProtocol() {}

ConnectionInfo StompProtocol::process_user_command(const std::string& input) {
        std::stringstream string_stream(input);
        std::string command;
        string_stream >> command;

        ConnectionInfo connection_info;

        std::lock_guard<std::mutex> lock(key_mutex); // Lock game_reports, frame_queue, and should_terminate_flag during processing

        if (command == "login") {

            if(is_connected) {
                std::cout << "You are already logged in. Please logout before trying to login again." << std::endl;
                return connection_info;
            }

            std::string host_port, username, passcode;
            string_stream >> host_port >> username >> passcode;
            current_user = username;
            size_t colon_pos = host_port.find(':');
            connection_info.host = host_port.substr(0, colon_pos);
            std::string port_str = host_port.substr(colon_pos + 1);
            // Convert port string to unsigned short
            connection_info.port = std::stoi(port_str);
            connection_info.should_connect = true;
            //frame_queue.push(StompFrame::create_connect_frame("stomp.cs.bgu.ac.il", username, passcode));
            frame_queue.push(StompFrame::create_connect_frame(connection_info.host, username, passcode));
            return connection_info;
        }
        if (!is_connected) {
                std::cout << "You must be logged in to do other commands." << std::endl;
                return connection_info;
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
            } else {
                std::cout << "You are not subscribed to channel " << game_name << std::endl;
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
            names_and_events parsed = parseEventsFile(file_path, current_user);
            std::string game_name = parsed.team_a_name + "_" + parsed.team_b_name;

            if (channel_to_sub_id.count(game_name)) {
                for (const auto& event : parsed.events) {
                    // Create a SEND frame for each event
                    // And add them to the queue.
                    StompFrame frame = StompFrame::create_send_frame("/" + game_name, event.to_string());
                    frame_queue.push(frame);
                }
            } else {
                std::cout << "You must join the channel " << game_name << " before reporting events for it." << std::endl;
            }
        }

        if (command == "summary") {
            std::string game_name, user, file_path;
            string_stream >> game_name >> user >> file_path;

            // Extract the events for the specified game and user
            if (game_reports.count(game_name) && game_reports[game_name].count(user)) {
                std::vector<Event>& events = game_reports[game_name][user];
                
                // Sort events by time (in case they are not sorted due to delay or network issues)
                std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
                    return a.get_time() < b.get_time();
                });

                // Maps to hold aggregated stats from all events 
                std::map<std::string, std::string> general_stats;
                std::map<std::string, std::string> team_a_stats;
                std::map<std::string, std::string> team_b_stats;
                
                // Aggregate stats from all events to maps
                for (const auto& event : events) {
                    for (auto const& it : event.get_game_updates()) general_stats[it.first] = it.second;
                    for (auto const& it : event.get_team_a_updates()) team_a_stats[it.first] = it.second;
                    for (auto const& it : event.get_team_b_updates()) team_b_stats[it.first] = it.second;
                }

                std::stringstream ss;
                std::string team_a = events[0].get_team_a_name();
                std::string team_b = events[0].get_team_b_name();
                
                // Generate game name according to format
                ss << team_a << " vs " << team_b << "\n";
                // Start writing stats in lexographic order since maps are sorted by key
                ss << "Game stats:\n";
                
                ss << "General stats:\n";
                for (auto const& it : general_stats) ss << it.first << ": " << it.second << "\n";
                
                ss << team_a << " stats:\n";
                for (auto const& it : team_a_stats) ss << it.first << ": " << it.second << "\n";

                ss << team_b << " stats:\n";
                for (auto const& it : team_b_stats) ss << it.first << ": " << it.second << "\n";
                // Now write all events in chronological order
                ss << "Game event reports:\n";
                for (const auto& event : events) {
                    ss << event.get_time() << " - " << event.get_name() << ":\n\n";
                    ss << event.get_discription() << "\n\n";
                }
                // Finally, write the summary to the specified file
                std::ofstream out_file(file_path);
                if (out_file.is_open()) {
                    out_file << ss.str();
                    out_file.close();
                }
            }
        }
        return connection_info;
    } // unlocks here

void StompProtocol::process_server_frame(const std::string& frame) {
    std::istringstream frame_stream(frame);

    StompFrame stomp_frame(frame);

    std::lock_guard<std::mutex> lock(key_mutex); // Lock game_reports and should_terminate_flag during processing

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

        //TODO: ***Maybe need to fix*** we know the game_name from the id and use id to channel to get the name and not destination
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
} // unlocks here

bool StompProtocol::should_terminate() {
    return should_terminate_flag;
}

bool StompProtocol::has_frames_to_send() {
    std::lock_guard<std::mutex> lock(key_mutex); // Lock frame_queue during processing
    return !frame_queue.empty();
}

StompFrame StompProtocol::get_next_frame() {
    std::lock_guard<std::mutex> lock(key_mutex); // Lock frame_queue during processing
    if (frame_queue.empty()) {
        return StompFrame("EMPTY", {}, ""); 
    }
    StompFrame frame = frame_queue.front();
    frame_queue.pop();
    return frame;
}
void StompProtocol::reset_termination() {
    std::lock_guard<std::mutex> lock(key_mutex); // Lock should_terminate_flag during processing
    should_terminate_flag = false;
}
