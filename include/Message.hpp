#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include "def.hpp"
#include <vector>
#include <string>
#include <atomic>

enum class MessageType {
    HEARTBEAT,
    CONNECT,
    DISCONNECT,
    REQTIME,
    REQHOST,
    REQCLILIST,
    REQSEND,
    ACK,
    FWD
};

class Message {
private:
    static std::atomic_uint16_t pakage_id_counter_;
    uint16_t pakage_id_;
    MessageType type_;
    uint8_t sender_id_;
    uint8_t receiver_id_;
    data_t data_;

public:
    /*
     * Empty constructor.
     * @param increase_pakage_id: Whether to increase the pakage id.
     */
    explicit Message(bool increase_pakage_id = false);
    /*
     * Constructor for parsing a message from a buffer.
     * @param buffer: The buffer to parse the message from.
     * @param size: The size of the buffer.
     */
    explicit Message(const void *buffer, ssize_t size);
    /*
     * Constructor for creating a message using the given parameters.
     * @param type: The type of the message.
     * @param sender_id: The id of the sender.
     * @param receiver_id: The id of the receiver.
     * @param data: The data of the message.
     * @param increase_pakage_id: Whether to increase the pakage id.
     */
    explicit Message(
        MessageType type,
        uint8_t sender_id,
        uint8_t receiver_id,
        const data_t &data = {},
        bool increase_pakage_id = true
    );
    /*
     * Copy constructor for Message.
     * @param other: The Message to copy.
     */
    Message(const Message &other);
    ~Message() {}

    // Getters
    uint16_t get_pakage_id() const;
    MessageType get_type() const;
    uint8_t get_sender_id() const;
    uint8_t get_receiver_id() const;
    data_t get_data() const;

    // Setters
    void set_pakage_id(uint16_t pakage_id);
    void set_pakage_id_to_next();
    void set_type(MessageType type);
    void set_sender_id(uint8_t sender_id);
    void set_receiver_id(uint8_t receiver_id);
    void set_data(const data_t &data);

    /*
     * Serializes the message into a buffer.
     * @param buffer: The buffer to serialize the message into.
     * @param size: The size of the buffer.
     * @return: The size of the serialized message.
     */
    ssize_t serialize(std::vector<uint8_t> &buffer) const;

    /*
     * Gets the size of the serialized message.
     * @return: The size of the serialized message.
     */
    ssize_t get_serialized_size() const;

    /*
     * Turns the message into a string.
     * @return: The string representation of the message.
     */
    std::string to_string() const;

    /*
     * Check if the message is valid.
     * @param buffer: The buffer to parse the message from.
     * @param size: The size of the buffer.
     * @return: If the message is valid, return the size of the message.
     *          Otherwise, return -1.
     */
    static ssize_t check_valid_message(const void *buffer, ssize_t size);

    Message& operator=(const Message& other);
};

inline bool check_afk(
    const Message &message,
    const send_res_t &result,
    uint8_t sender_id = SERVER_ID
) {
    return message.get_type() == MessageType::ACK  &&
           message.get_pakage_id() == result.first &&
           message.get_sender_id() == sender_id;
}

#endif
