#include <string>
#include "StompFrame.h"
#include <sstream>

    StompFrame::StompFrame() : command(""), headers(), body("") {}

    StompFrame::StompFrame(const std::string& command, const std::map<std::string, std::string>& headers, const std::string& body)
        : command(command), headers(headers), body(body) {}

    StompFrame::StompFrame(const std::string& frame_str) : command(""), headers(), body(""){
        std::stringstream frame_stream(frame_str);
        std::string line;

        std::getline(frame_stream, line);
        
        // Remove carriage character if present (if the server is windows-based it might be added)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        command = line;

        while (std::getline(frame_stream, line) && line != "" && line != "\r") {
            if (!line.empty() && line.back() == '\r') {line.pop_back();}
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

    StompFrame StompFrame::create_connect_frame(const std::string& host, const std::string& username, const std::string& passcode){
        std::string frame = "CONNECT\n";
        frame += "accept-version:1.2\n";
        frame += "host:" + host + "\n";
        frame += "login:" + username + "\n";
        frame += "passcode:" + passcode + "\n\n";
        frame += '\0';
        return StompFrame(frame);
    };

    StompFrame StompFrame::create_subscribe_frame(const std::string& destination, int sub_id, int receipt_id){

        std::string frame = "SUBSCRIBE\n";
        frame += "destination:" + destination + "\n";
        frame += "id:" + std::to_string(sub_id) + "\n";
        frame += "receipt:" + std::to_string(receipt_id) + "\n\n";
        frame += '\0';
        return StompFrame(frame);
    };

    StompFrame StompFrame::create_unsubscribe_frame(int sub_id, int receipt_id){
        std::string frame = "UNSUBSCRIBE\n";
        frame += "id:" + std::to_string(sub_id) + "\n";
        frame += "receipt:" + std::to_string(receipt_id) + "\n\n";
        frame += '\0';
        return StompFrame(frame);
    };

    StompFrame StompFrame::create_send_frame(const std::string& destination, const std::string& body){
        std::string frame = "SEND\n";
        frame += "destination:" + destination + "\n\n";
        frame += body + '\n';
        frame += '\0';
        return StompFrame(frame);
    };

    StompFrame StompFrame::create_disconnect_frame(int receipt_id){
        std::string frame = "DISCONNECT\n";
        frame += "receipt:" + std::to_string(receipt_id) + "\n\n";
        frame += '\0';
        return StompFrame(frame);
    };

    std::string StompFrame::to_string() const {
        std::string frame_str = command + "\n";

        for (auto const& header : headers) {
            frame_str += header.first + ":" + header.second + "\n";
        }
        // Blank line to separate headers from body
        frame_str += "\n";

        frame_str += body;
        frame_str += '\0';
        return frame_str;
    };
