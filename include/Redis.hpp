#pragma once

#include "Server.hpp"
#include <algorithm>

// Response status codes
enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2, // Not found
};

// Structure to hold a parsed request command
struct Request {
    std::vector<std::string> command;

    std::string lowerCaseCommand(int index = 0) const {
        if(!command.empty())
            std::transform(command[0].begin(), command[0].end(), command[0].begin(), ::tolower);
        
        return command[0];
    }
};

// Response structure
struct Response {
    uint32_t status = RES_OK;
    std::vector<uint8_t> data;
};

// Placeholder for the key-value data store
static std::unordered_map<std::string, std::string> g_data;

class RedisServer : public Server {
public:
    using Server::Server; // Inherit constructor

private:
    /**
     * @brief Handles incoming requests from clients.
     * @param conn The client connection that sent the request.
     * @param request The actual request data received from the client.
     */
    void onRequest(Connection& conn, const std::string& request) override;

    /**
     * @brief Parses a 32-bit unsigned integer from a raw byte buffer.
     * @details This function reads 4 bytes from the current buffer position,
     * interprets them as a little-endian uint32_t, and advances the
     * buffer cursor.
     *
     * @param cursor A reference to a pointer indicating the current position
     * in the buffer. It will be advanced by 4 bytes on success.
     * @param buffer_end A pointer to the end of the buffer to prevent over-reading.
     * @param value A reference to store the parsed integer.
     * @return true if 4 bytes were successfully read, false otherwise.
     */
    bool parseUInt32(const char*& cursor, const char* buffer_end, uint32_t& value);

    /**
     * @brief Parses a complete client command from a raw network message.
     * @details The message is expected to follow the Redis protocol: a count
     * of strings, followed by length-prefixed strings.
     *
     * @param raw_data The complete, raw message buffer received from the client.
     * @param parsed_request A reference to a Request object to populate with
     * the parsed command and its arguments.
     * @return 0 on success, or a negative value on parsing failure (e.g.,
     * bad format, trailing garbage).
     */
    int32_t parseRequest(const std::string& raw_data, Request& parsed_request);

    /**
     * @brief Executes a parsed command and populates a Response object.
     * @details This function contains the core application logic (e.g., get,
     * set, del) for handling client requests.
     *
     * @param request The parsed request object containing the command to execute.
     * @param response A reference to a Response object that will be populated
     * with the result of the command execution.
     */
    void executeRequest(const Request& request, Response& response);

    /**
     * @brief Serializes a Response object into a byte vector for network transmission.
     * @details This converts the status code and data from the Response object
     * into the length-prefixed format expected by the client.
     *
     * @param response The response object to serialize.
     * @param output_buffer The byte vector where the serialized response will
     * be appended.
     */
    void serializeResponse(const Response& response, std::vector<uint8_t>& output_buffer);
};