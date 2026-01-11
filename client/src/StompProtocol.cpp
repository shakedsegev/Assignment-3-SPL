#include <string>
#include <map>

class StompProtocol {

public:
    StompProtocol();
    ~StompProtocol();
std::string stomp_protocol::process_user_command(const std::string& input) {
    std::stringstream ss(input);
    std::string command;
    ss >> command;

    if (command == "login") {
        std::string host_port, username, password;
        ss >> host_port >> username >> password;
        return StompFrame::create_connect_frame("stomp.cs.bgu.ac.il", username, password);
    }

    if (command == "join") {
        std::string game_name;
        ss >> game_name;
        int sub_id = subscription_id_counter++;
        int receipt_id = receipt_id_counter++;
        channel_to_sub_id[game_name] = sub_id;
        receipt_to_action[receipt_id] = "Joined channel " + game_name;
        return StompFrame::create_subscribe_frame("/" + game_name, sub_id, receipt_id);
    }

    if (command == "exit") {
        std::string game_name;
        ss >> game_name;
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
        ss >> file_path;
        handle_report(file_path);
        return "";
    }

    if (command == "summary") {
        std::string game_name, user, file_path;
        ss >> game_name >> user >> file_path;
        save_summary_to_file(game_name, user, file_path);
        return "";
    }

    return "";
}


};