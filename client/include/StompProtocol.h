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

    public:
        StompProtocol();
        ~StompProtocol();

        std::string process_user_command(const std::string& input);

        std::map<std::string, Event> parse_event_from_body(const std::string& body);
        void parse_server_frame(const std::string& frame, std::string& command, std::map<std::string, std::string>& headers, std::string& body);
        void process_server_frame(const std::string& frame);
};
