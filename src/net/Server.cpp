#include <net/Server.hpp>

/* ======= Private methods ======= */

void Server::accept() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = ::accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);

    if(client_fd < 0) {
        if(errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "::accept() error: " << strerror(errno) << std::endl;
        
        return; // No new connection or an error occurred
    }
    
    std::unique_ptr<Connection> client = std::make_unique<Connection>(client_fd, client_addr);
    std::cout << "New client connected (ID:" << client_fd << "): " << client->getAddress() << std::endl;

    clients[client_fd] = std::move(client);
}

bool Server::process(Connection &client) {
    if(client.incoming.size() < 4)
        return false; // Not enough data to read the message header
    
    uint32_t payload_len = 0;
    // Copy the 4 raw bytes from the header buffer into our integer variable
    // to decode the length of the upcoming message payload.
    memcpy(&payload_len, client.incoming.data(), 4);
    if(payload_len > net::MAX_MSG) {
        std::cerr << "Error: Received message length (" << payload_len
                  << ") exceeds max size (" << net::MAX_MSG << ").\n";
        client.want_close = true;
        return false;
    }

    if(4 + payload_len > client.incoming.size())
        return false; // Not enough data to read the entire message
    
    std::string request (
            reinterpret_cast<const char*>(client.incoming.data() + 4), 
            payload_len
        );
    
    // Process the request by the application-specific handler
    onRequest(client, request);
    
    // Remove the processed message from the incoming buffer
    client.consumeIncoming(4 + payload_len);

    if (!client.outgoing.empty()) {
        client.want_write = true;
    }

    return true; // Successfully processed one request
}

void Server::recv(Connection& client) {
    uint8_t buffer[64 * 1024];
    ssize_t bytes_recv = ::recv(client.fd, buffer, sizeof(buffer), 0);
    
    if(bytes_recv <= 0) {
        if(bytes_recv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return; // No new data
        }

        std::string error = std::string("::recv() error: ") + strerror(errno);
        std::string closed = "Client (ID:" + std::to_string(client.fd) + ") closed connection";

        std::cerr << (bytes_recv == 0 ? closed : error) << std::endl;
        client.want_close = true;
        return;
    }

    // Add the received data to the incoming buffer
    client.appendIncoming(buffer, bytes_recv);

    // Process as many requests as possible
    while(process(client)) {}

    // If the outgoing buffer is not empty, send the data
    if(!client.outgoing.empty()) {
        client.want_read = false;
        client.want_write = true;
        // Try to send immediately to reduce latency
        send(client);
    }
}

void Server::send(Connection& client) {
    if(client.outgoing.empty()){
        client.want_read = true;
        client.want_write = false;
        
        return;
    }

    // Try to send the data
    ssize_t bytes_sent = ::send(client.fd, client.outgoing.data(), client.outgoing.size(), 0);
    
    // If the send failed, close the connection
    if(bytes_sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return; // Not an error, socket buffer is full
        
        std::cerr << "::send() error: " << strerror(errno) << std::endl;
        client.want_close = true;
        return;
    }
    
    // Remove the sent data from the outgoing buffer
    client.consumeOutgoing(bytes_sent);

    if(client.outgoing.empty()) {
        client.want_read = true;
        client.want_write = false;
    }
}

/* ======= Protected methods ======= */

void Server::onRequest(Connection& client, const std::string& request) {
    // Default behavior is to just echo the request.
    std::cout << "Client (fd=" << client.fd << ") says: " << request << std::endl;
    client.appendOutgoing(request);
}

/* ======= Public methods ======= */

Server::Server(uint16_t PORT): PORT(PORT) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
        net::die("socket() error");
    
    int reuse = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        net::die("setsockopt() error");
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(net::IP_ADDRESS.c_str());
    server_addr.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        net::die("bind() error");
    
    if(listen(server_fd, SOMAXCONN) < 0)
        net::die("listen() error");
    
    net::set_nonblocking(server_fd);
    std::cout << "Server listening on port " << PORT << " ..." << std::endl;
}

void Server::run() {
    std::vector<struct pollfd> poll_fds;
    std::vector<int> fd_to_remove;

    while(true) {
        poll_fds.clear();

        // Add the server socket to the poll list
        struct pollfd server_pfd = {
            .fd = server_fd,
            .events = POLLIN,
            .revents = 0
        };
        poll_fds.push_back(server_pfd);

        // Add all the client connection sockets
        for(const auto& client: clients) {
            struct pollfd client_pfd = {
                .fd = client.second->fd,
                .events = POLLERR,
                .revents = 0
            };

            if(client.second->want_read)
                client_pfd.events |= POLLIN;

            if(client.second->want_write)
                client_pfd.events |= POLLOUT;

            poll_fds.push_back(client_pfd);
        }

        // Wait for events
        int events = poll(poll_fds.data(), (nfds_t) poll_fds.size(), -1);
        if(events < 0) {
            if(errno != EINTR)
                std::cerr << "poll() error: " << strerror(errno) << std::endl;
            continue;
        }

        // Process server events
        for(size_t i=0; i<poll_fds.size(); ++i) {
            // no event in this socket
            if(poll_fds[i].revents == 0) continue;

            int fd = poll_fds[i].fd;

            // New connection
            if(fd == server_fd) {
                if (poll_fds[i].revents & POLLIN) accept();
                continue;
            }
            
            // find the client connection
            auto it = clients.find(fd);
            if(it == clients.end()) continue;

            Connection& client = *it->second;
            if(poll_fds[i].revents & POLLIN ) assert(client.want_read ), recv(client);
            if(poll_fds[i].revents & POLLOUT) assert(client.want_write), send(client);
            if(poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) client.want_close = true;

            if(client.want_close) fd_to_remove.push_back(fd);
        }

        // Remove closed connections
        for(int fd: fd_to_remove) {
            std::cout << "Closing connection (ID:" << fd << ") "<< std::endl;
            close(fd);
            clients.erase(fd);
        }
        
        fd_to_remove.clear();
    }
}

Server::~Server() {
    close(server_fd);
}