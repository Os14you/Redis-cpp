#include <Client.hpp>
#include <Deserialization.hpp>
#include <cstring>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./redis-cli <command> [args...]" << std::endl;
        return 1;
    }

    try {
        Client client("127.0.0.1", 6379); // Connect to the Redis port

        std::vector<std::string> request_cmd;
        for (int i = 1; i < argc; ++i) {
            request_cmd.push_back(argv[i]);
        }

        std::cout << "> ";
        for (const auto &s: request_cmd) { std::cout << s << " "; }
        std::cout << std::endl;

        client.send(request_cmd);
        Buffer res = client.recv();

        if(res.empty()) {
            std::cerr << "Received empty response from server." << std::endl;
        } else {
            printResponse(res, 0, 0);
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}