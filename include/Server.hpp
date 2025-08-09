#pragma once

#include "Network.hpp"
#include <unordered_map>
#include <memory>
#include <cassert>
#include <iostream>

using net::Connection;

class Server {
private:
    int server_fd;
    uint16_t PORT;
    // Using a map for efficient fd-based lookups and unique_ptr for memory management
    std::unordered_map<int, std::unique_ptr<Connection>> clients;

    /**
     * @brief Accepts incoming client connections and adds them to the clients map.
     * @return void
     */
    void accept();

    /**
     * @brief Processes a client connection by sending and receiving messages.
     * @param client The client connection to process.
     * @return bool
     */
    bool process(Connection& client);

    /**
     * @brief Sends a message to a client connection.
     * @param client The client connection to send the message to.
     * @return void
     */
    void send(Connection& client);

    /**
     * @brief Receives a message from a client connection.
     * @param client The client connection to receive the message from.
     * @return void
     */
    void recv(Connection& client);

protected:
    /**
     * @brief Handles an incoming request from a client.
     * @details This virtual function is intended to be overridden by derived classes to provide custom request handling logic.
     * @param client  The client connection that sent the request.
     * @param request The actual request data received from the client.
     */
    virtual void onRequest(Connection& client, const std::string& request);

public:
    /**
     * @brief Constructor for the Server class.
     * @param port The port number to listen on.
     */
    Server(uint16_t port);

    /**
     * @brief Runs the server and handles client connections.
     * @return void
     */
    void run();

    /**
     * @brief Destructor for the Server class.
     * @details Closes the server socket and cleans up any resources.
     * @note This destructor is virtual and can be overridden in derived classes.
     */
    virtual~Server();
};