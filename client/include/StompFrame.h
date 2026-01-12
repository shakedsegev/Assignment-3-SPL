#include <string>
#include <map>

class StompFrame {
    public:
        std::string command;
        std::map<std::string, std::string> headers;
        std::string body;

        StompFrame();
        StompFrame(const std::string& command, const std::map<std::string, std::string>& headers, const std::string& body);
        StompFrame(const std::string& frame_str);

        static std::string create_connect_frame(const std::string& host, const std::string& username, const std::string& passcode);
        static std::string create_subscribe_frame(const std::string& destination, int sub_id, int receipt_id);
        static std::string create_unsubscribe_frame(int sub_id, int receipt_id);
        static std::string create_send_frame(const std::string& destination, const std::string& body);
        static std::string create_disconnect_frame(int receipt_id);

        std::string to_string() const;

    private:
               




};