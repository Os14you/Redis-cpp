#pragma once

#include <vector>    // For std::vector (safer buffer)
#include <cstring>   // For memcpy
#include <iostream>  // For std::cout, std::cerr
#include <unistd.h>  // for close()
#include <cstdint>   // For int32_t
#include <cassert>   // For assert()

/**
 * @brief Handles one complete client request based on a length-prefixed protocol.
 * @param connection_fd The file descriptor for the connected client's socket.
 * @return True on success, false on failure (e.g., read/write error, bad message).
 */
bool handle_one_request(int connection_fd) {
    // This protocol consists of:
    // 1. A 4-byte header indicating the payload length.
    // 2. The payload (the actual message).

    // --- 1. Read the Message Header ---
    char header_buffer[4];
    if (read_full(connection_fd, header_buffer, 4) != 0) {
        std::cerr << "Error reading message header or client disconnected.\n";
        return false;
    }

    uint32_t payload_length = 0;
    // Copy the 4 raw bytes from the header buffer into our integer variable
    // to decode the length of the upcoming message payload.
    memcpy(&payload_length, header_buffer, sizeof(payload_length));

    // --- 2. Validate Payload Length and Read the Payload ---
    const size_t max_payload_size = 4096;
    if (payload_length > max_payload_size) {
        std::cerr << "Error: Received message length (" << payload_length
                  << ") exceeds max size (" << max_payload_size << ").\n";
        return false;
    }

    // Use a std::vector for a dynamically-sized, safe buffer.
    std::vector<char> payload_buffer(payload_length);
    if (read_full(connection_fd, payload_buffer.data(), payload_length) != 0) {
        std::cerr << "Error reading message payload.\n";
        return false;
    }

    // --- 3. Process the Request ---
    // For this example, we just print the client's message.
    // The data might not be null-terminated, so we specify the length.
    std::cout << "Client says: ";
    std::cout.write(payload_buffer.data(), payload_length);
    std::cout << std::endl;

    // --- 4. Prepare and Send the Response ---
    const std::string reply_message = "world";
    
    // Create a buffer for the entire response (header + payload).
    const uint32_t reply_length = reply_message.length();
    std::vector<char> write_buffer(sizeof(reply_length) + reply_length);

    // Copy the 4-byte length header into the beginning of the buffer.
    memcpy(write_buffer.data(), &reply_length, sizeof(reply_length));
    
    // Copy the reply message payload right after the header.
    memcpy(write_buffer.data() + sizeof(reply_length), reply_message.c_str(), reply_length);
    
    // Send the complete response.
    if (write_all(connection_fd, write_buffer.data(), write_buffer.size()) != 0) {
        std::cerr << "Error sending reply.\n";
        return false;
    }

    return true; // Request handled successfully
}

/**
 * @brief Reliably reads an exact number of bytes from a file descriptor.
 * @param file_descriptor The socket file descriptor to read from.
 * @param buffer The buffer where the read data will be stored.
 * @param bytes_to_read The total number of bytes to read.
 * @return 0 on success, -1 on error or if the connection is closed before all bytes are read.
 * @note This function now handles EINTR, where a system call is interrupted by a signal.
 */
int32_t read_full(int file_descriptor, char *buffer, size_t bytes_to_read) {
    while (bytes_to_read > 0) {
        ssize_t bytes_actually_read = read(file_descriptor, buffer, bytes_to_read);

        if (bytes_actually_read < 0) {
            // A real error occurred, but check if it was just an interruption.
            if (errno == EINTR) {
                continue; // The call was interrupted by a signal, retry.
            }
            // A non-recoverable error occurred.
            return -1;
        }
        
        if (bytes_actually_read == 0) {
            // The peer closed the connection (EOF). We cannot read more.
            return -1; 
        }

        assert(static_cast<size_t>(bytes_actually_read) <= bytes_to_read);

        bytes_to_read -= static_cast<size_t>(bytes_actually_read);
        buffer += bytes_actually_read;
    }

    return 0;
}

/**
 * @brief Reliably writes an exact number of bytes to a file descriptor.
 * @param file_descriptor The socket file descriptor to write to.
 * @param buffer The buffer containing the data to be written.
 * @param bytes_to_write The total number of bytes to write.
 * @return 0 on success, -1 on error.
 * @note This function now handles EINTR, where a system call is interrupted by a signal.
 */
int32_t write_all(int file_descriptor, const char *buffer, size_t bytes_to_write) {
    while (bytes_to_write > 0) {
        ssize_t bytes_actually_written = write(file_descriptor, buffer, bytes_to_write);

        if (bytes_actually_written < 0) {
            // A real error occurred, but check if it was just an interruption.
            if (errno == EINTR) {
                continue; // The call was interrupted by a signal, retry.
            }
            // A non-recoverable error occurred.
            return -1;
        }
        
        if (bytes_actually_written == 0) {
            // This shouldn't normally happen with write, but if it does,
            // it indicates a problem.
            return -1;
        }

        assert(static_cast<size_t>(bytes_actually_written) <= bytes_to_write);

        bytes_to_write -= static_cast<size_t>(bytes_actually_written);
        buffer += bytes_actually_written;
    }

    return 0;
}