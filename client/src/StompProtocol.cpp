
#include <string>
#include <map>
#include <sstream>
#include "StompProtocol.h"
#include "StompFrame.h"

StompProtocol::StompProtocol() : subscription_id_counter(1), receipt_id_counter(1), is_connected(false) {}
StompProtocol::~StompProtocol() {}

StompFrame StompProtocol::process_user_command(const std::string& input) {
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
            return StompFrame();//Not finished
        }

        if (command == "summary") {
            std::string game_name, user, file_path;
            string_stream >> game_name >> user >> file_path;
            // save_summary_to_file(game_name, user, file_path);
            return StompFrame();//Not finished
        }

        return StompFrame(); //Not finished yet
    }



    std::string StompProtocol::trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }


    std::string StompProtocol::extract_value(const std::string& line, const std::string& prefix) {
        return trim(line.substr(prefix.length()));
    } 

    std::map<std::string, Event> StompProtocol::parse_event_from_body(const std::string& body) {
        std::stringstream stringstream(body);
        std::string line;
        
        // Prefixes for extracting fields
        const std::string P_USER = "user: ";
        const std::string P_TEAM_A = "team a: ";
        const std::string P_TEAM_B = "team b: ";
        const std::string P_EVENT = "event name: ";
        const std::string P_TIME = "time: ";

        std::string user, team_a, team_b, event_name, description;
        int time = 0;
        std::map<std::string, std::string> gen_stats, t_a_stats, t_b_stats;
        std::string current_section = "";

        while (std::getline(stringstream, line)) {
            line = trim(line);
            if (line.empty()) continue;

            // Extracting main fields
            if (line.find(P_USER) == 0) user = extract_value(line, P_USER);
            else if (line.find(P_TEAM_A) == 0) team_a = extract_value(line, P_TEAM_A);
            else if (line.find(P_TEAM_B) == 0) team_b = extract_value(line, P_TEAM_B);
            else if (line.find(P_EVENT) == 0) event_name = extract_value(line, P_EVENT);
            else if (line.find(P_TIME) == 0) time = std::stoi(extract_value(line, P_TIME));

            // Identify section transitions 
            else if (line == "general game updates:") current_section = "general";
            else if (line == "team a updates:") current_section = "team_a";
            else if (line == "team b updates:") current_section = "team_b";
            else if (line == "description:") current_section = "description";
            
            else {
                if (current_section == "description") {
                    description += line + "\n";
                } else {
                    // Parsing key-value pairs within statistical sections
                    size_t colon_pos = line.find(':');
                    if (colon_pos != std::string::npos) {
                        std::string key = trim(line.substr(0, colon_pos));
                        std::string val = trim(line.substr(colon_pos + 1));

                        if (current_section == "general") gen_stats[key] = val;
                        else if (current_section == "team_a") t_a_stats[key] = val;
                        else if (current_section == "team_b") t_b_stats[key] = val;
                    }
                }
            }
        }

    std::map<std::string, Event> result;
    result.emplace(user, Event(team_a, team_b, event_name, time, gen_stats, t_a_stats, t_b_stats, description));
    return result;
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
            std::map<std::string, Event> event_map = parse_event_from_body(stomp_frame.body);
            if (!event_map.empty()) {
                auto it = event_map.begin();
                std::string reporter_name = it->first;
                Event event = it->second;
                game_reports[game_name][reporter_name].push_back(event);
            }
        } else if (stomp_frame.command == "ERROR") {
            std::cout << stomp_frame.to_string() << std::endl;
            is_connected = false;
            should_terminate_flag = true;
        }
        
    }
