#include <string>
#include "StompFrame.h"

class StompFrame {
    public:
        static std::string create_connect_frame(const std::string& host, const std::string& username, const std::string& password){
            std::string frame = "CONNECT\n";
            frame += "accept-version:1.2\n";
            frame += "host:" + host + "\n";
            frame += "login:" + username + "\n";
            frame += "passcode:" + password + "\n\n";
            frame += '\0';
            return frame;
        };

        static std::string create_subscribe_frame(const std::string& destination, int sub_id, int receipt_id){

            std::string frame = "SUBSCRIBE\n";
            frame += "destination:" + destination + "\n";
            frame += "id:" + std::to_string(sub_id) + "\n";
            frame += "receipt:" + std::to_string(receipt_id) + "\n\n";
            frame += '\0';
            return frame;
        };

        static std::string create_unsubscribe_frame(int sub_id, int receipt_id){
            std::string frame = "UNSUBSCRIBE\n";
            frame += "id:" + std::to_string(sub_id) + "\n";
            frame += "receipt:" + std::to_string(receipt_id) + "\n\n";
            frame += '\0';
            return frame;
        };

        static std::string create_send_frame(const std::string& destination, const std::string& body){
            std::string frame = "SEND\n";
            frame += "destination:" + destination + "\n\n";
            frame += body + '\n';
            frame += '\0';
            return frame;
        };
        
        static std::string create_disconnect_frame(int receipt_id){
            std::string frame = "DISCONNECT\n";
            frame += "receipt:" + std::to_string(receipt_id) + "\n\n";
            frame += '\0';
            return frame;
        };
};