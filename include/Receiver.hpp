#ifndef __RECEIVER_HPP__
#define __RECEIVER_HPP__

#include "def.hpp"
#include "Message.hpp"
#include <mutex>

class Receiver {
private:
    int socketfd_;
    uint8_t self_id_;
    std::vector<uint8_t> buffer_;

public:
    /*
     * Constructor.
     * @param socket: The socket to receive messages on.
     * @param self_id: The id of the receiver.
     */
    explicit Receiver(int socket, uint8_t self_id);
    /*
     * Destructor.
     */
    ~Receiver();

    /*
     * Change self_id_.
     * @param self_id: The new self_id.
     */
    void set_self_id(uint8_t self_id);

    /*
     * Receive a message.
     * @param message: The message to receive.
     * @return: The number of bytes received.
     */
    size_t receive(Message &message);
};

#endif
