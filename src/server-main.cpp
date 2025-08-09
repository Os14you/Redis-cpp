#include <Server.hpp>

class RedisServer : public Server {
public:
    using Server::Server; // Inherit constructor

protected:
    void onRequest(Connection& conn, const std::string& request) override {
        std::cout << "Redis Server received from (ID:" << conn.fd << "): " << request << std::endl;
        
        if (request == "PING") {
            conn.appendOutgoing("+PONG\r\n");
        } else {
            conn.appendOutgoing("-ERR unknown command\r\n");
        }
    }
};

int main() {
    try {
        RedisServer server(6379); // Default Redis port
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}