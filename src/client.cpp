#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

int main(int argc, char *argv[]) {
    // create socket with TCP protocol & IPv4
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // connect to server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(6379);
    if(connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        return 2;
    }

    std::cout << "Client connected to server..." << std::endl;

    // buffer for receiving data
    char buffer[1024];
    
    // receive the initial welcome message from server
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if(bytes_received < 0) {
        std::cerr << "Error receiving welcome message from server" << std::endl;
        close(client_fd);
        return 3;
    }

    buffer[bytes_received] = '\0';
    std::cout << "Message from server: " << buffer;

    // get command from the user
    std::string command;
    std::cout << "Enter command: ";
    std::getline(std::cin, command);

    // This ensures the server can correctly parse the end of the command.
    command += "\r\n";

    // send command to server
    if(send(client_fd, command.c_str(), command.length(), 0) < 0) {
        std::cerr << "Error sending command to server" << std::endl;
        close(client_fd);
        return 4;
    }

    // Clear the buffer before receiving the next message
    memset(buffer, 0, sizeof(buffer));

    // receive response from server
    bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if(bytes_received < 0) {
        std::cerr << "Error receiving response from server" << std::endl;
        close(client_fd);
        return 5;
    }

    buffer[bytes_received] = '\0';
    std::cout << "Response from server: " << buffer;

    // close socket
    close(client_fd);
    return 0;
}
