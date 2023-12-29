#ifndef __RECEIVER_HPP__
#define __RECEIVER_HPP__

#include "def.hpp"
#include "Message.hpp"
#include <mutex>
#include <sys/epoll.h>
#include <queue>

class Receiver {
private:
    std::mutex mutex_;
    int sockfd_;
    int epollfd_;
    std::atomic_char lose_heart_beat_;
    std::vector<epoll_event> events_;
    uint8_t self_id_;
    std::vector<uint8_t> buffer_;
    // It seems that message_queue_ is not needed
    // to be protected by another mutex.
    std::vector<uint8_t> remaining_buffer_;
    std::queue<Message> message_queue_;

public:
    /*
     * Constructor.
     * @param sockfd: The sockfd to receive messages on.
     * @param self_id: The id of the receiver.
     */
    explicit Receiver(int sockfd, uint8_t self_id);
    ~Receiver() {}

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
    ssize_t receive(Message &message);

    // operations on lose_heart_beat_
    void inc_lost_heart_beat();
    void set_lost_heart_beat(uint8_t lost_heart_beat);
    void reset_lost_heart_beat();
    uint8_t get_lost_heart_beat();
};

#endif
