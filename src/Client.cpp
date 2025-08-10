#include <Client.hpp>

/* ======= Private methods ======= */

void Client::read_full(uint8_t* buffer, size_t len) {
    while (len > 0) {
        ssize_t rv = ::recv(fd, buffer, len, 0);
        if (rv <= 0) {
            if (rv == 0)
                // EOF: The server closed the connection unexpectedly
                throw std::runtime_error("Unexpected EOF from server");
            else
                net::die("recv()");
        }

        assert((size_t) rv <= len);
        len -= (size_t)rv;
        buffer += rv;
    }
}

void Client::write_all(const uint8_t* buffer, size_t len) {
    while (len > 0) {
        ssize_t rv = ::send(fd, buffer, len, 0);
        if (rv <= 0)
            net::die("send()");

        assert((size_t) rv <= len);
        len -= (size_t)rv;
        buffer += rv;
    }
}


/* ======= Public methods ======= */

Client::Client(const std::string& host, const uint16_t& port) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        net::die("socket()");
    
    struct sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = inet_addr(host.c_str()) },
        .sin_zero = {}
    };

    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) != 0)
        net::die("connect()");
    
    std::cout << "Connected to server at " << host << ":" << port << std::endl;
}

void Client::send(const std::vector<std::string>& cmd) {
    std::vector<uint8_t> payload;
    uint32_t nstr = cmd.size();
    payload.insert(payload.end(), reinterpret_cast<uint8_t*>(&nstr), reinterpret_cast<uint8_t*>(&nstr) + 4);

    for (const auto& s : cmd) {
        uint32_t len = s.length();
        payload.insert(payload.end(), reinterpret_cast<uint8_t*>(&len), reinterpret_cast<uint8_t*>(&len) + 4);
        payload.insert(payload.end(), s.begin(), s.end());
    }

    std::vector<uint8_t> final_message;
    uint32_t total_len = payload.size();
    final_message.insert(final_message.end(), reinterpret_cast<uint8_t*>(&total_len), reinterpret_cast<uint8_t*>(&total_len) + 4);
    final_message.insert(final_message.end(), payload.begin(), payload.end());

    write_all(final_message.data(), final_message.size());
}

std::vector<uint8_t> Client::recv() {
    uint32_t len = 0;
    read_full(reinterpret_cast<uint8_t*> (&len), 4);
    
    if (len > net::MAX_MSG)
        throw std::runtime_error("Received message too long");

    std::vector<uint8_t> buffer(len);
    read_full(buffer.data(), len);
    
    return buffer;
}

Client::~Client() {
    if (fd >= 0) {
        close(fd);
        std::cout << "Connection closed." << std::endl;
    }
}