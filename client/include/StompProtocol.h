#pragma once

#include "ConnectionHandler.h"
#include "event.h"

// TODO: implement the STOMP protocol
class StompProtocol {
    private:
        int subscription_id_counter;
        int receipt_id_counter;
        bool is_connected;
        std::map<int, std::string> receipt_to_action;
        std::map<std::string, int> channel_to_sub_id;
        // Map: Game Name -> (Map: Reporter Name -> Vector of Events ordered by time)
        std::map<std::string, std::map<std::string, std::vector<Event>>> game_reports;

    public:
        bool should_terminate_flag;

        StompProtocol();
        ~StompProtocol();

        StompFrame process_user_command(const std::string& input);

        std::string trim(const std::string &str);
        std::string extract_value(const std::string &line, const std::string &prefix);
        std::map<std::string, Event> parse_event_from_body(const std::string &body);
        void process_server_frame(const std::string& frame);
};
