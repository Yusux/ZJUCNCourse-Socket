#include "Sender.hpp"
#include <sys/socket.h>

// FOR CLIENTS ONLY
Sender::Sender(int socket, unsigned char self_id) {
    socketfd_ = socket;
    self_id_ = self_id;
    buffer_.resize(MAX_BUFFER_SIZE);
}

Sender::~Sender() = default;

void Sender::set_self_id(unsigned char self_id) {
    self_id_ = self_id;
}
#include <iostream>
send_res_t Sender::send_connect_request(std::string name) {
    std::cout << "send_connect_request" << std::endl;
    data_t data;
    data.push_back(name);
    Message message(MessageType::CONNECT, self_id_, SERVER_ID, data);
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_disconnect_request(unsigned char receiver_id) {
    std::cout << "send_disconnect_request" << std::endl;
    Message message(MessageType::DISCONNECT, self_id_, receiver_id);
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_time() {
    std::cout << "send_request_time" << std::endl;
    Message message(MessageType::REQTIME, self_id_, SERVER_ID);
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_host() {
    std::cout << "send_request_host" << std::endl;
    Message message(MessageType::REQHOST, self_id_, SERVER_ID);
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_client_list() {
    std::cout << "send_request_client_list" << std::endl;
    Message message(MessageType::REQCLILIST, self_id_, SERVER_ID);
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_send(unsigned char receiver_id,
                                     std::string msg_string) {
    std::cout << "send_request_send" << std::endl;
    data_t data;
    data.push_back(msg_string);
    Message message(MessageType::REQSEND, self_id_, receiver_id, data);
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

// FOR SERVER AND CLIENTS
send_res_t Sender::send_acknowledge(unsigned char pakage_id,
                                    unsigned char receiver_id,
                                    const data_t &data) {
    std::cout << "send_acknowledge" << std::endl;
    Message message(MessageType::ACK, self_id_, receiver_id, data);
    message.set_pakage_id(pakage_id);
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_forward(Message message) {
    std::cout << "send_forward" << std::endl;
    message.set_type(MessageType::FWD);
    message.set_pakage_id_to_next();
    size_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}
