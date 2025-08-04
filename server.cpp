#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>


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

    // accept incoming connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if(client_fd < 0) {
        std::cerr << "Error accepting incoming connection" << std::endl;
        return 4;
    }

    // display client address
    std::cout << "Client connected from " << inet_ntoa(client_addr.sin_addr) << ":" 
              << ntohs(client_addr.sin_port) << std::endl;

    // send message to client
    std::string message = "Hello client! This is a message from the Redis server.";
    if(send(client_fd, message.c_str(), message.length(), 0) < 0) {
        std::cerr << "Error sending message to client" << std::endl;
        return 5;
    } 

    // receive message from client
    char buffer[1024];
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    } else {
        std::cerr << "Error receiving message from client" << std::endl;
        return 6;
    }

    // print message from client
    std::cout << "Message from client: " << buffer << std::endl;
    
    // respond to client
    if(strcmp(lowerIt(buffer), "ping") == 0)
        send(client_fd, "pong", 4, 0);
    else if(lowerIt(buffer) == "exit")
        send(client_fd, "+Ok .. goodbye", 14, 0);
    else
        send(client_fd, "+Error: unknown command ... exiting", 22, 0);
    
    // close socket
    close(client_fd);
    close(server_fd);
    return 0;
}