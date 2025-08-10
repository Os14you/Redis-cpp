#include <Redis.hpp>

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