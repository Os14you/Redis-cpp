#include <Client.hpp>

int main() {
    try {
        Client client("127.0.0.1", 6379); // Connect to the Redis port

        std::vector<std::string> requests = { "PING", "GET mykey", "hello" };

        for (const auto& req : requests) {
            client.send(req);
            std::cout << "Sent: " << req << std::endl;
            std::vector<uint8_t> res = client.recv();
            std::cout << "Received: " << std::string(res.begin(), res.end()) << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}