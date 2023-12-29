#include "Receiver.hpp"
#include <sys/socket.h>
#include <fcntl.h>

Receiver::Receiver(int socket, uint8_t self_id) {
    sockfd_ = socket;
    self_id_ = self_id;
    buffer_.resize(MAX_BUFFER_SIZE);

    // prepare epoll
    epollfd_ = epoll_create(MAX_EPOLL_EVENTS);
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = sockfd_;
    epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd_, &event);
    events_.resize(MAX_EPOLL_EVENTS);

    // change the socket to non-blocking
    int oldSocketFlag = fcntl(sockfd_, F_GETFL, 0);
    int newSocketFlag = oldSocketFlag | O_NONBLOCK;
    fcntl(sockfd_, F_SETFL,  newSocketFlag);

    // // set keepalive
    // int keepalive = 1;
    // int keepidle = 60;
    // int keepinterval = 5;
    // int keepcount = 3;
    // setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
}

void Receiver::set_self_id(uint8_t self_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    self_id_ = self_id;
}

ssize_t Receiver::receive(Message &message) {
    // TODO: handle the error and timeout
    std::lock_guard<std::mutex> lock(mutex_);
    if (message_queue_.empty()) {
        // use epoll_wait to wait for the socket to be readable
        int nfds;
        while ((nfds = epoll_wait(epollfd_, events_.data(), MAX_EPOLL_EVENTS, TIMEOUT)) == 0);
        // if epoll_wait returns 0, it means that the timeout expires
        // if epoll_wait returns -1, it means that an error occurs
        if (nfds == -1) {
            std::string error_message = "epoll_wait error: nfds = " + std::to_string(nfds) +
                                        ", errno = " + std::to_string(errno);
            perror(error_message.c_str());
            return 0;
        }

        // if epoll_wait returns a positive number, it means that the socket is readable
        ssize_t size = recv(sockfd_, reinterpret_cast<void *>(buffer_.data()), MAX_BUFFER_SIZE, 0);
        // if recv returns -1, it means that an error occurs
        // if recv returns 0, it means that the peer has closed the connection
        if (size == ssize_t(-1) || size == 0) {
            std::string error_message = "recv error: size = " + std::to_string(size) +
                                        ", errno = " + std::to_string(errno);
            perror(error_message.c_str());
            return 0;
        }

        // append the new data to the remaining buffer
        remaining_buffer_.insert(remaining_buffer_.end(), buffer_.begin(), buffer_.begin() + size);
        ssize_t length;
        while ((length = Message::check_valid_message(remaining_buffer_.data(), remaining_buffer_.size())) > 0) {
            // get the message
            Message message_to_push(remaining_buffer_.data(), length);
            // push the message to the queue
            message_queue_.push(message_to_push);
            // remove the message from the remaining buffer
            remaining_buffer_.erase(remaining_buffer_.begin(), remaining_buffer_.begin() + length);
        }
    }

    // get the message from the queue
    if (!message_queue_.empty()) {
        message = message_queue_.front();
        message_queue_.pop();
        return message.get_serialized_size();
    } else {
        return 0;
    }
}
