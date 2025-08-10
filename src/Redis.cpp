#include <Redis.hpp>

/* ====== Private methods ====== */

void RedisServer::onRequest(Connection& conn, const std::string& request) {
    Request parsed_request;
    Response response;

    // 1. Parse the raw binary data from the client.
    if (parseRequest(request, parsed_request) != 0) {
        // If parsing fails, create a proper Response object for the error.
        response.status = RES_ERR;
        const char* msg = "ERR Protocol error";
        response.data.assign(reinterpret_cast<const uint8_t*>(msg), reinterpret_cast<const uint8_t*>(msg) + strlen(msg));
    } else {
        // 2. If parsing succeeds, execute the command.
        executeRequest(parsed_request, response);
    }

    // 3. ALWAYS serialize the final response object.
    serializeResponse(response, conn.outgoing);

    // If parsing failed, we still want to close the connection.
    if (response.status != RES_OK && response.status != RES_NX) {
        if (std::string(response.data.begin(), response.data.end()).find("Protocol error") != std::string::npos) {
             conn.want_close = true;
        }
    }
}

bool RedisServer::parseUInt32(const char*& cursor, const char* buffer_end, uint32_t& value) {
    if(cursor + 4 > buffer_end)
        return false; // Not enough data to read a uint32_t

    memcpy(&value, cursor, 4);
    cursor += 4;

    return true; // Successfully read a uint32_t
}

int32_t RedisServer::parseRequest(const std::string& raw_data, Request& parsed_request) {
    const char* cursor = raw_data.data();
    const char* buffer_end = raw_data.data() + raw_data.size();

    uint32_t num_strings = 0;
    if(!parseUInt32(cursor, buffer_end, num_strings))
        return -1; // Failed to parse the number of strings

    const size_t K_MAX_ARGS = 1024;
    if(num_strings > K_MAX_ARGS)
        return -2; // Too many arguments

    while(num_strings--) {
        uint32_t str_len = 0;
        if(!parseUInt32(cursor, buffer_end, str_len))
            return -1; // Failed to parse string length
        
        if(cursor + str_len > buffer_end)
            return -3; // Not enough data for the string
        
        parsed_request.command.emplace_back(cursor, str_len);
        cursor += str_len;
    }

    if(cursor != buffer_end)
        return -4; // Trailing garbage after the command
    
    return 0; // Successfully parsed the request
}

void RedisServer::executeRequest(const Request& request, Response& response) {
    if(request.command.empty()) {
        response.status = RES_ERR;
        const char* error_msg = "ERR empty command";
        response.data.assign(
            reinterpret_cast<const uint8_t*>(error_msg), 
            reinterpret_cast<const uint8_t*>(error_msg) + strlen(error_msg)
        );
        return;
    }

    const std::string& cmd = request.lowerCaseCommand();

    if(cmd == "ping") {
        response.status = RES_OK;
        const char* pong_msg = "+PONG\r\n";
        response.data.assign(
            reinterpret_cast<const uint8_t*>(pong_msg), 
            reinterpret_cast<const uint8_t*>(pong_msg) + strlen(pong_msg)
        );
    } else if(cmd == "get") {
        if(request.command.size() < 2) {
            response.status = RES_ERR;
            const char* error_msg = "ERR wrong number of arguments for 'get' command";
            response.data.assign(
                reinterpret_cast<const uint8_t*>(error_msg), 
                reinterpret_cast<const uint8_t*>(error_msg) + strlen(error_msg)
            );
            return;
        }

        auto it = g_data.find(request.command[1]);
        if(it != g_data.end()) {
            response.data.assign(it->second.begin(), it->second.end());
            response.status = RES_OK;
        }
        else
            response.status = RES_NX; // Not found
    
    } else if(cmd == "set") {
        if(request.command.size() != 3) {
            response.status = RES_ERR;
            const char* error_msg = "ERR wrong number of arguments for 'set' command";
            response.data.assign(
                reinterpret_cast<const uint8_t*>(error_msg), 
                reinterpret_cast<const uint8_t*>(error_msg) + strlen(error_msg)
            );
            return;
        }

        g_data[request.command[1]] = request.command[2];
        response.status = RES_OK;

        const char* ok_msg = "+OK\r\n";
        response.data.assign(
            reinterpret_cast<const uint8_t*>(ok_msg), 
            reinterpret_cast<const uint8_t*>(ok_msg) + strlen(ok_msg)
        );

    } else if(cmd == "del") {
        if(request.command.size() < 2) {
            response.status = RES_ERR;
            const char* error_msg = "ERR wrong number of arguments for 'del' command";
            response.data.assign(
                reinterpret_cast<const uint8_t*>(error_msg),
                reinterpret_cast<const uint8_t*>(error_msg) + strlen(error_msg)
            );
            return;
        }

        g_data.erase(request.command[1]);
        response.status = RES_OK;
        const char* ok_msg = "+OK\r\n";
        response.data.assign(
            reinterpret_cast<const uint8_t*>(ok_msg),
            reinterpret_cast<const uint8_t*>(ok_msg) + strlen(ok_msg)
        );

    } else {
        response.status = RES_ERR;
        std::string error_msg = "ERR unknown '" + cmd + "'";
        response.data.assign(
            reinterpret_cast<const uint8_t*>(error_msg.c_str()), 
            reinterpret_cast<const uint8_t*>(error_msg.c_str()) + error_msg.size()
        );
    }
}

void RedisServer::serializeResponse(const Response& response, std::vector<uint8_t>& output_buffer) {
    uint32_t payload_len = 4 + (uint32_t)response.data.size();

    // 1. Append the total payload length (4 bytes).
    output_buffer.insert(
        output_buffer.end(), 
        reinterpret_cast<const uint8_t*>(&payload_len), 
        reinterpret_cast<const uint8_t*>(&payload_len) + 4
    );

    // 2. Append the status code (4 bytes).
    output_buffer.insert(
        output_buffer.end(),
        reinterpret_cast<const uint8_t*>(&response.status),
        reinterpret_cast<const uint8_t*>(&response.status) + 4
    );

    // 3. Append the actual response data (e.g., the value from a 'get').
    output_buffer.insert(
        output_buffer.end(),
        response.data.begin(),
        response.data.end()
    );
}