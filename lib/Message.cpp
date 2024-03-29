#include "Message.hpp"
#include <stdexcept>

std::atomic_uint16_t Message::pakage_id_counter_ = 0;

Message::Message(bool increase_pakage_id) {
    if (increase_pakage_id) {
        pakage_id_ = pakage_id_counter_++;
        if (pakage_id_counter_ == 0) {
            pakage_id_ = pakage_id_counter_++;
        }
    } else {
        pakage_id_ = pakage_id_counter_;
    }
    type_ = MessageType::HEARTBEAT;
    sender_id_ = 0;
    receiver_id_ = 0;
    data_ = {};
}

Message::Message(const void *buffer, ssize_t size) {
    if (size < 6) {
        throw std::runtime_error("Message buffer too small.");
    }
    const uint8_t *buffer_ptr = reinterpret_cast<const uint8_t *>(buffer);

    // get the packet id, type, sender id and receiver id
    pakage_id_ = *(reinterpret_cast<const uint16_t *>(buffer));
    type_ = (MessageType)buffer_ptr[2];
    sender_id_ = buffer_ptr[3];
    receiver_id_ = buffer_ptr[4];
    uint8_t num_data = buffer_ptr[5];

    // get the data, every data is a vector of uchar
    ssize_t data_size = size - 6;
    const uint8_t *data_ptr = buffer_ptr + 6;
    for (int i = 0; i < num_data; i++) {
        if (data_size < 1) {
            throw std::runtime_error("Message buffer error.");
        }
        uint8_t data_length = data_ptr[0];
        if ((data_length + 1) > data_size) {
            throw std::runtime_error("Message buffer overflow.");
        }
        data_.push_back(std::string(data_ptr + 1, data_ptr + 1 + data_length));
        data_ptr += data_length + 1;
        data_size -= data_length + 1;
    }
}

Message::Message(
    MessageType type,
    uint8_t sender_id,
    uint8_t receiver_id,
    const data_t &data,
    bool increase_pakage_id
) {
    // overflow is fine, it's unsigned
    if (increase_pakage_id) {
        pakage_id_ = pakage_id_counter_++;
        if (pakage_id_counter_ == 0) {
            pakage_id_ = pakage_id_counter_++;
        }
    } else {
        pakage_id_ = pakage_id_counter_;
    }
    type_ = type;
    sender_id_ = sender_id;
    receiver_id_ = receiver_id;
    data_ = data;
}

Message::Message(const Message &other) {
    pakage_id_ = other.pakage_id_;
    type_ = other.type_;
    sender_id_ = other.sender_id_;
    receiver_id_ = other.receiver_id_;
    data_ = other.data_;
}


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

ssize_t Message::serialize(std::vector<uint8_t> &buffer) const {
    if (data_.size() > 255) {
        throw std::runtime_error("Message data too large.");
    }

    // clear the buffer
    buffer.clear();

    // add the packet id, type, sender id and receiver id
    buffer.resize(2);
    *(reinterpret_cast<uint16_t *>(buffer.data())) = pakage_id_;
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

ssize_t Message::get_serialized_size() const {
    ssize_t size = 6;
    for (auto &data : data_) {
        size += data.size() + 1;
    }
    return size;
}

ssize_t Message::check_valid_message(const void *buffer, ssize_t size) {
    if (size < 6) {
        return -1;
    }
    const uint8_t *buffer_ptr = reinterpret_cast<const uint8_t *>(buffer);

    // get the packet id, type, sender id and receiver id
    uint8_t num_data = buffer_ptr[5];

    // get the data, every data is a vector of uchar
    ssize_t data_size = size - 6;
    const uint8_t *data_ptr = buffer_ptr + 6;
    for (int i = 0; i < num_data; i++) {
        if (data_size < 1) {
            return -1;
        }
        uint8_t data_length = data_ptr[0];
        data_ptr += data_length + 1;
        data_size -= data_length + 1;
    }

    return data_ptr - buffer_ptr;
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

Message& Message::operator=(const Message& other) {
    if (this != &other) {
        pakage_id_ = other.pakage_id_;
        type_ = other.type_;
        sender_id_ = other.sender_id_;
        receiver_id_ = other.receiver_id_;
        data_ = other.data_;
    }
    return *this;
}
