#include <Client.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>

int main() {
    try {
        Client client("127.0.0.1", 6379); // Connect to the Redis port

        std::vector<std::vector<std::string>> requests = {
            {"SET", "OSAMA", "best"},
            {"GET", "OSAMA"},
            {"DEL", "OSAMA"},
            {"GET", "OSAMA"},
            {"PING"},
            {"ECHO", "Hello, Redis!"} // Note: ECHO is not implemented yet
        };

        for (const auto& req : requests) {
            std::cout << "Sending: ";
            for(const auto& s : req) { std::cout << s << " "; }
            std::cout << std::endl;

            client.send(req);

            std::vector<uint8_t> res = client.recv();

            // The response payload must have at least 4 bytes for the status code.
            if (res.size() < 4) {
                std::cerr << "Invalid response received" << std::endl;
                continue;
            }

            uint32_t status;
            // The status code is at the BEGINNING of the response buffer (offset 0).
            memcpy(&status, res.data(), 4);

            std::cout << "Received Status: " << status;
            // The data (if any) starts right AFTER the 4-byte status code.
            if (res.size() > 4) {
                 std::cout << ", Data: " << std::string(res.begin() + 4, res.end());
            }

            std::cout << std::endl << "---" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}