#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace net {
    
    const int PORT = 6379;
    const std::string IP_ADDRESS = "127.0.0.1";
    const size_t MAX_MSG = 32 << 20; // 32 MB maximum message size

    /** 
     * @brief Prints error message and exits with error code 1
     * @param msg Error message
     * @throws std::runtime_error
     */
    void die(const char* msg);

    /**
     * @brief Sets the file descriptor to be non-blocking
     * @param fd File descriptor
     * @returns void
     */
    void set_nonblocking(int fd);

    class Connection {
    private:
        // client address
        struct sockaddr_in addr;
        
    public:
        int fd = -1;

        bool want_read  = false;
        bool want_write = false;
        bool want_close = false;

        // Buffers for incoming and outgoing data
        std::vector<uint8_t> incoming;
        std::vector<uint8_t> outgoing;

        /**
         * @brief Constructor for the Connection class
         * @param client_fd File descriptor for the client
         * @returns void
         * @throws std::runtime_error
         */
        Connection(const int& client_fd, const struct sockaddr_in& client_addr);

        /**
         * @brief Appends data to the outgoing buffer to be sent
         * @param data Data to append
         * @param len Length of the data
         * @returns void
         */
        void appendOutgoing(const uint8_t* data, const size_t& len);

        /**
         * @brief Appends string to the outgoing buffer to be sent
         * @param str String to append
         * @returns void
         */
        void appendOutgoing(const std::string& str);

        /**
         * @brief Appends data to the incoming buffer to be processed
         * @param data Data to append
         * @param len Length of the data
         * @returns void
         */
        void appendIncoming(const uint8_t* data, const size_t& len);

        /**
         * @brief Consumes data from the incoming buffer
         * @param len Length of the data to consume
         * @returns void
         */
        void consumeIncoming(size_t len);

        /**
         * @brief Consumes data from the outgoing buffer
         * @param len Length of the data to consume
         * @returns void
         */
        void consumeOutgoing(size_t len);

        /**
         * @brief Returns the address of the client
         * @returns std::string
         */
        std::string getAddress() const;
    };
}