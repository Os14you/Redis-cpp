#include <Client.hpp>

int main() {
    try {
        Client client("127.0.0.1", 6379); // Connect to the Redis port

        std::vector<std::vector<std::string>> requests = {
            {"SET", "OSAMA", "is the best"},
            {"GET", "OSAMA"},
            {"DEL", "OSAMA"},
            {"GET", "OSAMA"},
            {"PING"},
            {"ECHO", "Hello, Redis!"}
        };

        for (const auto& req : requests) {
            std::cout << "Sending: ";
            for(const auto& s : req) { std::cout << s << " "; }
            std::cout << std::endl;

            // The API is now much cleaner!
            client.send(req);

            std::vector<uint8_t> res = client.recv();

            // The response from the server is also length-prefixed with a status
            if (res.size() < 8) {
                std::cerr << "Invalid response received" << std::endl;
                continue;
            }

            uint32_t status;
            memcpy(&status, res.data() + 4, 4);

            std::cout << "Received Status: " << status;
            if (res.size() > 8)
                 std::cout << ", Data: " << std::string(res.begin() + 8, res.end());
            
            std::cout << std::endl << "---" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}