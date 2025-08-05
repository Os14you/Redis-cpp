#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

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

    // receive message from server
    char buffer[1024];
    if(recv(client_fd, buffer, sizeof(buffer), 0) < 0) {
        std::cerr << "Error receiving response from server" << std::endl;
        return 3;
    }
    std::cout << "Message from server: " << buffer << std::endl;

    // get command from the user
    std::string command;
    std::cout << "Enter command: ";
    std::getline(std::cin, command);

    // send command to server
    if(send(client_fd, command.c_str(), command.length(), 0) < 0) {
        std::cerr << "Error sending command to server" << std::endl;
        return 4;
    }

    memset(buffer, 0, sizeof(buffer));
    // receive response from server
    if(recv(client_fd, buffer, sizeof(buffer), 0) < 0) {
        std::cerr << "Error receiving response from server" << std::endl;
        return 5;
    }
    std::cout << "Response from server: " << buffer << std::endl;

    // close socket
    close(client_fd);
    return 0;
}