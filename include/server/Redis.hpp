#pragma once

#include "../net/Server.hpp"
#include "../core/HashTable.hpp"
#include "../core/ZSet.hpp"
#include "../common/Serialization.hpp"
#include <variant>
#include <algorithm>

// Structure to hold a parsed request command
struct Request {
    std::vector<std::string> command;
    
    /**
     * @brief Returns a lowercase copy of a command part.
     * @param index The index of the command part to convert (default is 0 for the command itself).
     * @return A new string containing the lowercase version of the command part.
     */
    std::string lowerCaseCommand(size_t index = 0) const {
        // Return an empty string if the command vector is empty or the index is out of bounds.
        if (command.empty() || index >= command.size()) {
            return "";
        }

        // 1. Create a copy of the original string.
        std::string lower_cmd = command[index];

        // 2. Transform the COPY to lowercase.
        //    Using a lambda is the safest way to call std::tolower.
        std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        // 3. Return the new lowercase string.
        return lower_cmd;
    }
};

struct DataEntry: public HashTable::Node {
    std::string key;
    std::variant<std::string, SortedSet> value;
};

class RedisServer : public Server {
public:
    RedisServer(uint16_t port);

private:
    HashTable dataStore;

    using CommandHandler = std::function<void(const Request&, Buffer&)>;
    std::unordered_map<std::string, CommandHandler> commandTable;

    void handleGet(const Request& request, Buffer& response);
    void handleSet(const Request& request, Buffer& response);
    void handleDel(const Request& request, Buffer& response);
    void handleZAdd(const Request& request, Buffer& response);
    void handleZRem(const Request& request, Buffer& response);
    void handleKeys(const Request& request, Buffer& response);
    void handlePing(const Request& request, Buffer& response);
    void handleZRange(const Request& request, Buffer& response);
    void handleUnknown(const Request& request, Buffer& response);

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
    void executeRequest(const Request& request, Buffer& response);

    static uint64_t stringHash(const std::string& str);
};