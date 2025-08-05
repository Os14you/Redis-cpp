#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <vector>
#include <map>

struct ClientConn {
    int file_descriptor = -1;
    struct sockaddr_in client_addr;
    std::string write_buffer;

    void printClientAddr() {
        std::cout << inet_ntoa(client_addr.sin_addr) << ":" << std::to_string(ntohs(client_addr.sin_port));
    }
};

std::string respond(char *command) {
    if(command == "ping") return "+PONG\r\n";
    if(command == "exit") return "+OK\r\n";
    return "-ERR unknown command\r\n";
}

void set_nonblocking(int file_descriptor) {
    int flags = fcntl(file_descriptor, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return;
    }
    if (fcntl(file_descriptor, F_SETFL, flags | O_NONBLOCK) == -1)
        perror("fcntl(F_SETFL)");
}

char* lowerIt(char *s) {
    for(int i = 0; s[i] != '\0'; i++)
        s[i] = tolower(s[i]);
    return s;
}

int main(int argc, char *argv[]) {
    // create socket with TCP protocol & IPv4
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // set socket to be reusable
    int reuse = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        std::cerr << "Error setting socket options for address to be reusable" << std::endl;
    
    // bind socket to port 6379
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(6379);
    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket to port 6379" << std::endl;
        return 2;
    }

    // listen for incoming connections
    if(listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "Error listening for incoming connections" << std::endl;
        return 3;
    }

    std::cout << "Server listening on port 6379..." << std::endl;

    // data structure to manage client connections
    std::map<int, ClientConn> client_connections;
    std::vector<struct pollfd> poll_descriptors;

    // set server socket to non-blocking mode & add it to poll
    set_nonblocking(server_fd);
    poll_descriptors.push_back({server_fd, POLLIN, 0});

    while(true) {
        // get events from all the file descriptors
        int events = poll(poll_descriptors.data(), poll_descriptors.size(), -1);
        if(events < 0) {
            std::cerr << "Error polling file descriptors" << std::endl;
            break;
        }

        // iterating over all the file descriptors
        for(size_t i = 0; i < poll_descriptors.size(); i++) {
            // no events in this file descriptor
            if(poll_descriptors[i].revents == 0) { continue; }

            // check for events in the server file descriptor
            if(poll_descriptors[i].fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if(client_fd < 0) {
                    std::cerr << "Error accepting incoming connection" << std::endl;
                    return 4;
                }

                set_nonblocking(client_fd);
                client_connections[client_fd] = {client_fd, client_addr, ""};
                poll_descriptors.push_back({client_fd, POLLIN, 0});
                std::cout << "Client connected from "; client_connections[client_fd].printClientAddr(); std::cout << std::endl;
            } else {
                // events in client file descriptor
                int client_fd = poll_descriptors[i].fd;

                // check if client sent data
                if(poll_descriptors[i].revents & POLLIN) {
                    char buffer[1024];
                    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if(bytes_received <= 0) {
                        if(bytes_received == 0) {
                            std::cout << "Client disconnected from "; client_connections[client_fd].printClientAddr(); std::cout << std::endl;
                        } else
                            std::cerr << "Error receiving data from client" << std::endl;
                        
                        // close client connection and remove from map and vector
                        close(client_fd);
                        client_connections.erase(client_fd);
                        for(auto it = poll_descriptors.begin(); it != poll_descriptors.end(); it++) {
                            if(it->fd == client_fd) {
                                poll_descriptors.erase(it);
                                break;
                            }
                        }
                        // decrement i to account for the removed element
                        --i;
                    } else {
                        buffer[bytes_received] = '\0';
                        std::cout << "Client (id: " << client_fd << ") sent: " << buffer << std::endl;
                        
                        // add the respond to the write buffer
                        client_connections[client_fd].write_buffer += respond(lowerIt(buffer));

                        // set the socket to be writable
                        poll_descriptors[i].events |= POLLOUT;
                    }
                }

                // Case 2: Client socket is ready for writing
                if (poll_descriptors[i].revents & POLLOUT) {
                    ClientConn& client = client_connections[client_fd];
                    if (!client.write_buffer.empty()) {
                        ssize_t bytes_sent = write(client_fd, client.write_buffer.c_str(), client.write_buffer.length());
                        if (bytes_sent < 0)
                            std::cerr << "Error sending data to client" << std::endl;
                        else
                            // Remove the sent data from the buffer
                            client.write_buffer.erase(0, bytes_sent);
                    }

                    // If the write buffer is now empty
                    if (client.write_buffer.empty())
                        poll_descriptors[i].events &= ~POLLOUT;
                }
            }

        }
    }

    // close the server socket
    close(server_fd);
    return 0;
}