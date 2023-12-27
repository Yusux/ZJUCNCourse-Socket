#include "Message.hpp"
#include <stdexcept>

uint16_t Message::pakage_id_counter_ = 0;

Message::Message(bool increase_pakage_id) {
    pakage_id_ = increase_pakage_id ? pakage_id_counter_++ : pakage_id_counter_;
    type_ = MessageType::BLANK;
    sender_id_ = 0;
    receiver_id_ = 0;
    data_ = {};
}

Message::Message(const void *buffer, size_t size) {
    if (size < 5) {
        throw std::runtime_error("Message buffer too small.");
    }
    const uint8_t *buffer_ptr = reinterpret_cast<const uint8_t *>(buffer);

    // get the packet id, type, sender id and receiver id
    pakage_id_ = buffer_ptr[0];
    type_ = (MessageType)buffer_ptr[1];
    sender_id_ = buffer_ptr[2];
    receiver_id_ = buffer_ptr[3];
    uint8_t num_data = buffer_ptr[4];

    // get the data, every data is a vector of uchar
    size_t data_size = size - 5;
    const uint8_t *data_ptr = buffer_ptr + 5;
    for (int i = 0; i < num_data; i++) {
        uint8_t data_length = data_ptr[0];
        if ((data_length + 1) > data_size) {
            throw std::runtime_error("Message buffer overflow.");
        }
        data_.push_back(std::string(data_ptr + 1, data_ptr + 1 + data_length));
        data_ptr += data_length + 1;
        data_size -= data_length + 1;
    }
}

Message::Message(MessageType type,
                 uint8_t sender_id,
                 uint8_t receiver_id,
                 const data_t &data,
                 bool increase_pakage_id) {
    // overflow is fine, it's unsigned
    pakage_id_ = increase_pakage_id ? pakage_id_counter_++ : pakage_id_counter_;
    type_ = type;
    sender_id_ = sender_id;
    receiver_id_ = receiver_id;
    data_ = data;
}

Message::Message(const Message &other) {
    pakage_id_ = pakage_id_counter_;
    type_ = other.type_;
    sender_id_ = other.sender_id_;
    receiver_id_ = other.receiver_id_;
    data_ = other.data_;
}

Message::~Message() = default;

uint16_t Message::get_pakage_id() const {
    return pakage_id_;
}

MessageType Message::get_type() const {
    return type_;
}

uint8_t Message::get_sender_id() const {
    return sender_id_;
}

uint8_t Message::get_receiver_id() const {
    return receiver_id_;
}

data_t Message::get_data() const {
    return data_;
}

void Message::set_pakage_id(uint16_t pakage_id) {
    pakage_id_ = pakage_id;
}

void Message::set_pakage_id_to_next() {
    pakage_id_ = pakage_id_counter_++;
}

void Message::set_type(MessageType type) {
    type_ = type;
}

void Message::set_sender_id(uint8_t sender_id) {
    sender_id_ = sender_id;
}

void Message::set_receiver_id(uint8_t receiver_id) {
    receiver_id_ = receiver_id;
}

void Message::set_data(const data_t &data) {
    data_ = data;
}

size_t Message::serialize(std::vector<uint8_t> &buffer) const {
    // clear the buffer
    buffer.clear();

    // add the packet id, type, sender id and receiver id
    buffer.push_back(pakage_id_);
    buffer.push_back((uint8_t)type_);
    buffer.push_back(sender_id_);
    buffer.push_back(receiver_id_);
    buffer.push_back((uint8_t)data_.size());

    // add the data, every data is a vector of uchar
    for (auto &data : data_) {
        buffer.push_back((uint8_t)data.size());
        buffer.insert(buffer.end(), data.begin(), data.end());
    }

    return buffer.size();
}

std::string Message::to_string() const {
    std::string str = "Message(";
    str += "pakage_id=" + std::to_string(pakage_id_);
    str += ", type=" + std::to_string((uint8_t)type_);
    str += ", sender_id=" + std::to_string(sender_id_);
    str += ", receiver_id=" + std::to_string(receiver_id_);
    str += ", data=[";
    for (auto &data : data_) {
        str += "\"" + data + "\", ";
    }
    str += "])";
    return str;
}
