#include "Client.hpp"
#include <stdexcept>
#include <iostream>

Client::Client(
    std::string name
) : name_(name), last_msg_(false) {  // in order not to increase the pakage id
    // Prepare the server_addr_.
    server_addr_.sin_family = AF_INET;

    // Not to create a socket until call connect_to_server().
    socketfd_ = -1;

    // Not to connect to the server until call connect_to_server().
    sender_ = nullptr;
    receiver_ = nullptr;

    // Not to get self_id_ until call connect_to_server().
    self_id_ = 0;
}

Client::~Client() {
    if (socketfd_ < 0) {
        // Not connected to the server.
        return;
    }

    // Try to disconnect from the server.
    try {
        disconnect_from_server();
    } catch (const std::exception &e) {
        // Error in disconnection, delete socketfd_ anyway.
        std::cerr << e.what() << std::endl;
        std::cerr << "Release the client anyway." << std::endl;
        // Close the socket.
        close(socketfd_);
        // Release the sender and the receiver.
        sender_ = nullptr;
        receiver_ = nullptr;
    }
}

bool Client::connect_to_server(in_addr_t addr, int port) {
    // Prepare the server_addr_.
    server_addr_.sin_port = htons(port);
    server_addr_.sin_addr.s_addr = addr;

    // Create a socket.
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        throw std::runtime_error("CONNECT REQUEST failed: failed to create a socket.");
    }

    // Connect to the server.
    if (connect(socketfd, cast_sockaddr_in(server_addr_), sizeof(server_addr_)) < 0) {
        close(socketfd);
        throw std::runtime_error("CONNECT REQUEST failed: failed to connect to the server.");
    }

    // Create a sender and a receiver.
    sender_ = std::make_unique<Sender>(socketfd, 0);
    receiver_ = std::make_unique<Receiver>(socketfd, 0);

    // Send a CONNECT REQUEST.
    send_res_t result = sender_->send_connect_request(name_);
    receiver_->receive(last_msg_);
    if (check_afk(last_msg_, result)) {
        // Successfully connected to the server.
        self_id_ = last_msg_.get_receiver_id();
        sender_->set_self_id(self_id_);
        receiver_->set_self_id(self_id_);
    } else {
        // Error in connection.
        close(socketfd);
        throw std::runtime_error("CONNECT REQUEST failed: error in connection.");
    }

    // Successfully connected to the server.
    socketfd_ = socketfd;
    return true;
}

bool Client::disconnect_from_server() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("DISCONNECT REQUEST failed: not connected to the server.");
    }

    // Send a DISCONNECT REQUEST.
    send_res_t result = sender_->send_disconnect_request();
    size_t size = receiver_->receive(last_msg_);
    std::cout << "size: " << size << std::endl;
    std::cout << "data.size(): " << last_msg_.get_data().size() << std::endl;
    std::cout << "last_msg_: " << last_msg_.to_string() << std::endl;
    if (!check_afk(last_msg_, result)) {
        std::cerr << "DISCONNECT REQUEST failed: error in connection." << std::endl;
        std::cerr << "Release the connection anyway." << std::endl;
    }

    // Close the socket.
    close(socketfd_);
    socketfd_ = -1;

    // Release the sender and the receiver.
    sender_ = nullptr;
    receiver_ = nullptr;

    std::cout << "# disconnected." << std::endl;

    return true;
}

std::string Client::get_time() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("DISCONNECT REQUEST failed: not connected to the server.");
    }

    // Send a REQUEST TIME.
    send_res_t result = sender_->send_request_time();
    receiver_->receive(last_msg_);
    if (!check_afk(last_msg_, result)) {
        throw std::runtime_error("REQUEST TIME failed: error in connection.");
    }

    // Get the time.
    data_t data = last_msg_.get_data();
    if (data.size() != 1) {
        throw std::runtime_error("REQUEST TIME failed: data size is not 1.");
    }
    return data[0];
}

std::string Client::get_name() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("DISCONNECT REQUEST failed: not connected to the server.");
    }

    // Send a REQUEST HOST.
    send_res_t result = sender_->send_request_host();
    receiver_->receive(last_msg_);
    if (!check_afk(last_msg_, result)) {
        throw std::runtime_error("REQUEST HOST failed: error in connection.");
    }

    // Get the name.
    data_t data = last_msg_.get_data();
    if (data.size() != 1) {
        throw std::runtime_error("REQUEST HOST failed: data size is not 1.");
    }
    return data[0];
}

data_t Client::get_client_list() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("DISCONNECT REQUEST failed: not connected to the server.");
    }

    // Send a REQUEST CLIENT LIST.
    send_res_t result = sender_->send_request_client_list();
    receiver_->receive(last_msg_);
    if (!check_afk(last_msg_, result)) {
        throw std::runtime_error("REQUEST CLIENT LIST failed: error in connection.");
    }

    // Get the client list.
    data_t data = last_msg_.get_data();
    /* In the data, one client occupies 4 elements:
     * 1. id
     * 2. name
     * 3. ip
     * 4. port
     * Different clients are separated by '\n'. (The last one is also followed by '\n'.)
     */
    if (data.size() % 5 != 0) {
        throw std::runtime_error("REQUEST CLIENT LIST failed: data size is not a multiple of 5.");
    }

    return data;
}

bool Client::send_message(unsigned char receiver_id, std::string content) {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("DISCONNECT REQUEST failed: not connected to the server.");
    }

    // Send a REQUEST SEND.
    send_res_t result = sender_->send_request_send(receiver_id, content);
    receiver_->receive(last_msg_);
    if (!check_afk(last_msg_, result)) {
        throw std::runtime_error("REQUEST SEND failed: error in connection.");
    }

    data_t data = last_msg_.get_data();
    if (data.size() != 0) {
        std::string error_msg = "REQUEST SEND failed: " + data[0];
        throw std::runtime_error(error_msg);
    }

    return true;
}

void Client::receive_message() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("DISCONNECT REQUEST failed: not connected to the server.");
    }

    Message message(false);
    while (receiver_->receive(message)) {
        // Print the message.
        std::cout << "# message: " << message.to_string() << std::endl;
        // Check the type of the message
        if (message.get_type () == MessageType::DISCONNECT) {
            // Send an ACK.
            sender_->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
            // Remove the connection.
            close(socketfd_);
            socketfd_ = -1;
            sender_ = nullptr;
            receiver_ = nullptr;
            std::cout << "# disconnected." << std::endl;
            break;
        } else if (message.get_type() == MessageType::FWD) {
            std::string content;
            data_t data = message.get_data();
            for (auto &str : data) {
                content += str;
                content += "$\n";
            }
            std::cout << "Message from client " << (int)message.get_sender_id() << ": " << content << std::endl;
            // Send an ACK.
            sender_->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
        }
        std::cout << "WAITING FOR MESSAGE" << std::endl;
    }
}

std::string Client::get_name() const {
    return name_;
}

unsigned char Client::get_self_id() const {
    return self_id_;
}

Message Client::get_last_msg() const {
    return last_msg_;
}
