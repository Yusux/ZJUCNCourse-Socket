#include "Sender.hpp"
#include <sys/socket.h>

// FOR CLIENTS ONLY
Sender::Sender(int socket, uint8_t self_id) {
    socketfd_ = socket;
    self_id_ = self_id;
    buffer_.resize(MAX_BUFFER_SIZE);
}

void Sender::set_self_id(uint8_t self_id) {
    self_id_ = self_id;
}

send_res_t Sender::send_connect_request(std::string name) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_t data;
    data.push_back(name);
    Message message(MessageType::CONNECT, self_id_, SERVER_ID, data);
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_disconnect_request(uint8_t receiver_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    Message message(MessageType::DISCONNECT, self_id_, receiver_id);
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_time() {
    std::lock_guard<std::mutex> lock(mutex_);
    Message message(MessageType::REQTIME, self_id_, SERVER_ID);
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_host() {
    std::lock_guard<std::mutex> lock(mutex_);
    Message message(MessageType::REQHOST, self_id_, SERVER_ID);
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_client_list() {
    std::lock_guard<std::mutex> lock(mutex_);
    Message message(MessageType::REQCLILIST, self_id_, SERVER_ID);
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_request_send(uint8_t receiver_id,
                                     std::string msg_string) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_t data;
    data.push_back(msg_string);
    Message message(MessageType::REQSEND, self_id_, receiver_id, data);
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

// FOR SERVER AND CLIENTS
send_res_t Sender::send_acknowledge(uint16_t pakage_id,
                                    uint8_t receiver_id,
                                    const data_t &data) {
    std::lock_guard<std::mutex> lock(mutex_);
    Message message(MessageType::ACK, self_id_, receiver_id, data);
    message.set_pakage_id(pakage_id);
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}

send_res_t Sender::send_forward(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    message.set_type(MessageType::FWD);
    message.set_pakage_id_to_next();
    ssize_t size = message.serialize(buffer_);
    size = send(socketfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return std::make_pair(message.get_pakage_id(), size);
}
