#include "Server.hpp"
#include <stdexcept>
#include <iostream>
#include <ctime>

ClientInfo::ClientInfo(
    std::string name,
    sockaddr_in addr,
    int socketfd,
    unsigned char id,
    Sender *sender,
    Receiver *receiver
) : socketfd_(socketfd), name_(name), addr_(addr), client_id_(id) {
    sender_ = std::unique_ptr<Sender>(sender);
    receiver_ = std::unique_ptr<Receiver>(receiver);
}

ClientInfo::~ClientInfo() {
    // Close the socket.
    close(socketfd_);
    // Release the sender and the receiver.
    sender_ = nullptr;
    receiver_ = nullptr;
}

std::string ClientInfo::get_name() {
    return name_;
}

sockaddr_in ClientInfo::get_addr() {
    return addr_;
}

Sender *ClientInfo::get_sender() {
    return sender_.get();
}

Receiver *ClientInfo::get_receiver() {
    return receiver_.get();
}

Server::Server(
    std::string name,
    in_addr_t addr,
    int port
) : name_(name), self_id_(SERVER_ID), last_msg_(false) {
    // Prepare the server_addr_.
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port);
    server_addr_.sin_addr.s_addr = addr;

    // Create a socket.
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        throw std::runtime_error("SERVER INIT failed: failed to create a socket.");
    }

    // Set the socket to be reusable.
    int opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("SERVER INIT failed: failed to set the socket to be reusable.");
    }

    // Bind the socket to the server address and port.
    if (bind(socketfd, cast_sockaddr_in(server_addr_), sizeof(server_addr_)) < 0) {
        close(socketfd);
        throw std::runtime_error("SERVER INIT failed: failed to bind the socket to the server address and port.");
    }

    // Listen for connections for maximum MAX_CLIENT_NUM clients.
    listen(socketfd, MAX_CLIENT_NUM);

    // Save the socket.
    socketfd_ = socketfd;
}

Server::~Server() {
    // Close all the client connections.
    while (!client_list_.empty()) {
        auto it = client_list_.begin();
        try {
            // Send a DISCONNECT REQUEST.
            send_res_t result = it->second->get_sender()->send_disconnect_request(it->first);
            Message response;
            it->second->get_receiver()->receive(response);
            if (!check_afk(response, result, it->first)) {
                throw std::runtime_error("CLIENT CLOSE REQUEST failed: error in connection.");
            }
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            std::cerr << "Release the client anyway." << std::endl;
        }
        client_list_.erase(client_list_.begin());
    }

    // Close the socket.
    close(socketfd_);
}

unsigned char Server::wait_for_client() {
    // Accept a connection for client.
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socketfd = accept(socketfd_, cast_sockaddr_in(client_addr), &client_addr_len);
    if (client_socketfd < 0) {
        throw std::runtime_error("SERVER WAIT FOR CLIENT failed: failed to accept a connection.");
    }

    // Receive a CONNECT REQUEST.
    Message request(false);
    Receiver *receiver = new Receiver(client_socketfd, SERVER_ID);
    Sender *sender = new Sender(client_socketfd, SERVER_ID);
    receiver->receive(request);
    // Check if the message is a valid CONNECT REQUEST.
    if (request.get_type() != MessageType::CONNECT ||
        request.get_receiver_id() != SERVER_ID) {
        throw std::runtime_error("SERVER WAIT FOR CLIENT failed: invalid connection request.");
    }

    // Get the name of the client.
    if (request.get_data().size() != 1) {
        throw std::runtime_error("SERVER WAIT FOR CLIENT failed: invalid connection request.");
    }
    std::string client_name = request.get_data()[0];

    // Create a client info.
    // Find a valid client id.
    unsigned char id = 1;
    while (client_list_.find(id) != client_list_.end()) {
        id++;
    }
    if (id == 0) {
        throw std::runtime_error("SERVER WAIT FOR CLIENT failed: too many clients.");
    }
    std::unique_ptr<ClientInfo> client_info = std::make_unique<ClientInfo>(
        client_name,
        client_addr,
        client_socketfd,
        id,
        sender,
        receiver
    );
    client_list_.insert(std::make_pair(id, std::move(client_info)));

    // Send a CONNECT RESPONSE.
    client_list_[id]->get_sender()->send_acknowledge(request.get_pakage_id(), id);

    return id;
}

void Server::receive_from_client(unsigned char client_id) {
    // Check if the client id is valid.
    if (client_list_.find(client_id) == client_list_.end()) {
        throw std::runtime_error("SERVER RECEIVE FROM CLIENT failed: invalid client id.");
    }
    Receiver *receiver = client_list_[client_id]->get_receiver();
    Sender *sender = client_list_[client_id]->get_sender();

    Message message(false);
    std::cout << "# waiting for message..." << std::endl;
    while (receiver->receive(message)) {
        // Print the message.
        std::cout << "# message: " << message.to_string() << std::endl;
        // check the type of the message
        if (message.get_type () == MessageType::DISCONNECT) {
            // Send an ACK.
            sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
            // Remove the client.
            client_list_.erase(client_id);
            break;
        } else if (message.get_type() == MessageType::REQSEND) {
            // Try to find the receiver.
            if (client_list_.find(message.get_receiver_id()) == client_list_.end()) {
                // Not found.
                data_t data;
                data.push_back("The receiver is not found.");
                std::cerr << "# The receiver is not found." << std::endl;
                sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id(), data);
                continue;
            }

            // Found, Send a FWD.
            send_res_t result = client_list_[message.get_receiver_id()]->get_sender()->send_forward(message);
            // Wait for an ACK from the receiver.
            Message response(false);
            client_list_[message.get_receiver_id()]->get_receiver()->receive(response);
            if (!check_afk(response, result, message.get_receiver_id())) {
                data_t data;
                data.push_back("Error in connection between the server and the receiver.");
                std::cerr << "# Error in connection between the server and the receiver." << std::endl;
                sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id(), data);
                continue;
            }
            // Send an ACK.
            sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
        } else if (message.get_type() == MessageType::REQCLILIST) {
            // Send a ACK.
            data_t data;
            /* In the data, one client occupies 4 elements:
             * 1. id
             * 2. name
             * 3. ip
             * 4. port
             * Different clients are separated by '\n'. (The last one is also followed by '\n'.)
             */
            for (auto it = client_list_.begin(); it != client_list_.end(); it++) {
                std::string id_str = std::to_string(it->first);
                std::string name_str = it->second->get_name();
                std::string ip_str = inet_ntoa(it->second->get_addr().sin_addr);
                std::string port_str = std::to_string(ntohs(it->second->get_addr().sin_port));
                data.push_back(id_str);
                data.push_back(name_str);
                data.push_back(ip_str);
                data.push_back(port_str);
                data.push_back("\n");
            }
            sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id(), data);
        } else if (message.get_type() == MessageType::REQTIME) {
            // Get timestamp.
            std::time_t timestamp = std::time(nullptr);
            std::string timestamp_str = std::to_string(timestamp);
            data_t data;
            data.push_back(timestamp_str);
            // Send a ACK.
            sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id(), data);
        } else if (message.get_type() == MessageType::REQHOST) {
            // Send a ACK.
            data_t data;
            data.push_back(name_);
            sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id(), data);
        }
        std::cout << "# waiting for message..." << std::endl;
    }
}

std::string Server::get_client_name(unsigned char client_id) {
    // Check if the client id is valid.
    if (client_list_.find(client_id) == client_list_.end()) {
        throw std::runtime_error("SERVER GET CLIENT NAME failed: invalid client id.");
    }

    return client_list_[client_id]->get_name();
}

sockaddr_in Server::get_client_addr(unsigned char client_id) {
    // Check if the client id is valid.
    if (client_list_.find(client_id) == client_list_.end()) {
        throw std::runtime_error("SERVER GET CLIENT ADDR failed: invalid client id.");
    }

    return client_list_[client_id]->get_addr();
}
