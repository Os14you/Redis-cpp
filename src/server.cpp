#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include <string>

// A struct to hold information about each connected client.
struct ClientConn {
    int file_descriptor = -1;
    struct sockaddr_in client_addr;
    std::string write_buffer;

    // Helper function to print the client's IP and port.
    void printClientAddr() {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        std::cout << ip_str << ":" << ntohs(client_addr.sin_port);
    }
};

// Generates a response based on a received command.
std::string respond(char *command) {
    if (strcmp(command, "ping") == 0) return "+PONG\r\n";
    if (strcmp(command, "exit") == 0) return "+OK\r\n";
    return "-ERR unknown command\r\n";
}

// Sets a file descriptor to non-blocking mode.
void set_nonblocking(int file_descriptor) {
    int flags = fcntl(file_descriptor, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return;
    }
    if (fcntl(file_descriptor, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
    }
}

// Converts a C-style string to lowercase.
char* lowerIt(char *s) {
    for(int i = 0; s[i] != '\0'; i++) {
        s[i] = tolower(s[i]);
    }
    return s;
}

int main(int argc, char *argv[]) {
    // 1. Setup the listening socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "Error setting socket options for address to be reusable" << std::endl;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(6379);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket to port 6379" << std::endl;
        return 2;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "Error listening for incoming connections" << std::endl;
        return 3;
    }

    std::cout << "Server listening on port 6379..." << std::endl;

    // Data structures for managing clients and poll file descriptors
    std::map<int, ClientConn> client_connections;
    std::vector<struct pollfd> poll_descriptors;

    // Add the server socket to the poll list to listen for new connections
    set_nonblocking(server_fd);
    poll_descriptors.push_back({server_fd, POLLIN, 0});

    // 2. The main event loop
    while (true) {
        if (poll(poll_descriptors.data(), poll_descriptors.size(), -1) < 0) {
            std::cerr << "Error polling file descriptors" << std::endl;
            break;
        }

        for (size_t i = 0; i < poll_descriptors.size(); i++) {
            if (poll_descriptors[i].revents == 0) {
                continue; // No events for this fd
            }

            // Event on the listening socket: a new client is connecting.
            if (poll_descriptors[i].fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_fd < 0) {
                    std::cerr << "Error accepting incoming connection" << std::endl;
                    continue;
                }

                set_nonblocking(client_fd);
                client_connections[client_fd] = {client_fd, client_addr, "Hello, welcome to the Server in C++!\r\n"};
                
                poll_descriptors.push_back({client_fd, POLLIN | POLLOUT, 0});

                std::cout << "Client connected from "; 
                client_connections[client_fd].printClientAddr(); 
                std::cout << std::endl;
            } else {
                // Event on a client socket.
                int client_fd = poll_descriptors[i].fd;

                // Handle connection errors or hangups first.
                if (poll_descriptors[i].revents & (POLLERR | POLLHUP)) {
                    std::cout << "Client connection error/hangup from ";
                    client_connections[client_fd].printClientAddr();
                    std::cout << std::endl;
                    
                    close(client_fd);
                    client_connections.erase(client_fd);
                    poll_descriptors.erase(poll_descriptors.begin() + i);
                    --i; // Adjust index after removal
                    continue;
                }

                // Handle incoming data from the client.
                if (poll_descriptors[i].revents & POLLIN) {
                    char buffer[1024];
                    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_received <= 0) { // 0 means clean disconnect, < 0 is an error.
                        std::cout << "Client disconnected from ";
                        client_connections[client_fd].printClientAddr();
                        std::cout << std::endl;

                        close(client_fd);
                        client_connections.erase(client_fd);
                        poll_descriptors.erase(poll_descriptors.begin() + i);
                        --i; // Adjust index after removal
                    } else {
                        buffer[bytes_received] = '\0';
                        
                        // Trim trailing newline characters for correct parsing.
                        char* p = strchr(buffer, '\r');
                        if (p) *p = '\0';
                        p = strchr(buffer, '\n');
                        if (p) *p = '\0';

                        std::cout << "Client (id: " << client_fd << ") sent: \"" << buffer << "\"" << std::endl;
                        
                        // Add the response to this client's write buffer.
                        client_connections[client_fd].write_buffer += respond(lowerIt(buffer));

                        // Signal that we want to write to this socket on the next poll() iteration.
                        poll_descriptors[i].events |= POLLOUT;
                    }
                }

                // Handle writing data to the client.
                if (poll_descriptors[i].revents & POLLOUT) {
                    ClientConn& client = client_connections[client_fd];
                    if (!client.write_buffer.empty()) {
                        ssize_t bytes_sent = write(client_fd, client.write_buffer.c_str(), client.write_buffer.length());
                        if (bytes_sent < 0) {
                            std::cerr << "Error sending data to client" << std::endl;
                        } else {
                            // Remove the sent data from the buffer.
                            client.write_buffer.erase(0, bytes_sent);
                        }
                    }

                    // If the write buffer is now empty, stop monitoring for writability.
                    if (client.write_buffer.empty()) {
                        poll_descriptors[i].events &= ~POLLOUT;
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
