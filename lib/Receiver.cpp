#include "Receiver.hpp"
#include <sys/socket.h>

Receiver::Receiver(int socket, unsigned char self_id) {
    socketfd_ = socket;
    self_id_ = self_id;
    buffer_.resize(MAX_BUFFER_SIZE);
}

Receiver::~Receiver() = default;

void Receiver::set_self_id(unsigned char self_id) {
    self_id_ = self_id;
}
#include <iostream>
size_t Receiver::receive(Message &message) {
    size_t size = recv(socketfd_, reinterpret_cast<void *>(buffer_.data()), MAX_BUFFER_SIZE, 0);
    if (size > 4) {
        message = Message(buffer_.data(), size);
    }
    std::cout << "receive: " << size << std::endl;
    return size;
}