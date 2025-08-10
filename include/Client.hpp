#pragma once

#include "Network.hpp"
#include <cassert>
#include <iostream>

/**
 * @brief Client class for handling network communication with a server
 * This class provides methods to connect to a server, send messages, and receive responses.
 */
class Client {
private:
    int fd = -1;

    /**
     * @brief Helper function to read exactly 'len' bytes into the buffer
     * @param buffer Pointer to the buffer where data will be read
     * @param len Number of bytes to read
     * @throws std::runtime_error if the server closes the connection unexpectedly
     * @returns void
     */
    void read_full(uint8_t* buffer, size_t len);

    /**
     * @brief Helper function to write exactly 'len' bytes from the buffer
     * @param buffer Pointer to the buffer containing data to write
     * @param len Number of bytes to write
     * @returns void
     */
    void write_all(const uint8_t* buffer, size_t len);

public:
    /**
     * @brief Constructor for the Client class
     * @param host Hostname or IP address of the server
     * @param port Port number of the server
     * @returns void
     */
    Client(const std::string& host, const uint16_t& port);

    /**
     * @brief Sends a command to the server, formatted as a vector of strings.
     * @details This is the primary method for sending commands. It serializes
     * the command vector into the length-prefixed format expected by the server.
     * @param cmd The command to send, with each part as an element in the vector.
     */
    void send(const std::vector<std::string>& cmd);

    /**
     * @brief Receives a message from the server
     * @returns Received message as a vector of bytes
     */
    std::vector<uint8_t> recv();

    /**
     * @brief Closes the connection to the server
     * @returns void
     */
    ~Client();
};