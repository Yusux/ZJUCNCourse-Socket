#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include "def.hpp"
#include <vector>
#include <string>

enum class MessageType {
    BLANK,
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
    static unsigned char pakage_id_counter_;
    unsigned char pakage_id_;
    MessageType type_;
    unsigned char sender_id_;
    unsigned char receiver_id_;
    data_t data_;

public:
    /*
     * Empty constructor.
     * @param increase_pakage_id: Whether to increase the pakage id.
     */
    explicit Message(bool increase_pakage_id = true);
    /*
     * Constructor for parsing a message from a buffer.
     * @param buffer: The buffer to parse the message from.
     * @param size: The size of the buffer.
     */
    explicit Message(const void *buffer, size_t size);
    /*
     * Constructor for creating a message using the given parameters.
     * @param type: The type of the message.
     * @param sender_id: The id of the sender.
     * @param receiver_id: The id of the receiver.
     * @param data: The data of the message.
     * @param increase_pakage_id: Whether to increase the pakage id.
     */
    explicit Message(MessageType type,
                     unsigned char sender_id,
                     unsigned char receiver_id,
                     const data_t &data = {},
                     bool increase_pakage_id = true);
    /*
     * Copy constructor for Message.
     * @param other: The Message to copy.
     */
    Message(const Message &other);
    /*
     * Destructor.
     */
    ~Message();

    // Getters
    unsigned char get_pakage_id() const;
    MessageType get_type() const;
    unsigned char get_sender_id() const;
    unsigned char get_receiver_id() const;
    data_t get_data() const;

    // Setters
    void set_pakage_id(unsigned char pakage_id);
    void set_pakage_id_to_next();
    void set_type(MessageType type);
    void set_sender_id(unsigned char sender_id);
    void set_receiver_id(unsigned char receiver_id);
    void set_data(const data_t &data);

    /*
     * Serializes the message into a buffer.
     * @param buffer: The buffer to serialize the message into.
     * @param size: The size of the buffer.
     * @return: The size of the serialized message.
     */
    size_t serialize(std::vector<unsigned char> &buffer) const;

    /*
     * Turns the message into a string.
     * @return: The string representation of the message.
     */
    std::string to_string() const;
};

inline bool check_afk(const Message &message, const send_res_t &result, unsigned char sender_id = SERVER_ID) {
    return message.get_type() == MessageType::ACK  &&
           message.get_pakage_id() == result.first &&
           message.get_sender_id() == sender_id;
}

#endif
