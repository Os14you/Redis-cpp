#include "Network.hpp"
using namespace net;

static void net::die(const char* msg) {
    throw std::runtime_error(std::string(msg) + ": " + strerror(errno));
}

static void net::set_nonblocking(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno)
        die("fcntl(F_GETFL)");

    errno = 0;
    (void) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (errno)
        die("fcntl(F_SETFL)");
}

Connection::Connection(const int& client_fd): fd(client_fd), want_read(true) {
    set_nonblocking(fd);
}

void Connection::appendIncoming(const uint8_t* data, const size_t& len) {
    incoming.insert(incoming.end(), data, data + len);
}

void Connection::appendOutgoing(const uint8_t* data, const size_t& len) {
    outgoing.insert(outgoing.end(), data, data + len);
}

void Connection::appendOutgoing(const std::string& str) {
    uint32_t len = str.length();
    appendOutgoing(reinterpret_cast<const uint8_t*>(&len), 4);
    appendOutgoing(reinterpret_cast<const uint8_t*>(str.data()), str.length());
}

void Connection::consumeIncoming(size_t len) {
    if(len > incoming.size())
        len = incoming.size();
    
    incoming.erase(incoming.begin(), incoming.begin() + len);
}

void Connection::consumeOutgoing(size_t len) {
    if(len > outgoing.size())
        len = outgoing.size();
    
    outgoing.erase(outgoing.begin(), outgoing.begin() + len);
}

std::string Connection::getAddress() const {
    return std::string(inet_ntoa(addr.sin_addr)) + ":" + std::to_string(ntohs(addr.sin_port));
}