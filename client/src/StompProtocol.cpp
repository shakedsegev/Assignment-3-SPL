
#include <string>
#include <map>
#include <sstream>
#include "../include/StompProtocol.h"
#include "../include/StompFrame.h"

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