#include "Server.hpp"
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <ctime>

ClientInfo::ClientInfo(
    std::string name,
    sockaddr_in addr,
    int socketfd,
    uint8_t id,
    Sender *sender,
    Receiver *receiver
) : socketfd_(socketfd), name_(name), addr_(addr), client_id_(id) {
    sender_ = std::unique_ptr<Sender>(sender);
    receiver_ = std::unique_ptr<Receiver>(receiver);
}

ClientInfo::~ClientInfo() {
    // Close the socket.
    close(socketfd_);
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
) : name_(name), self_id_(SERVER_ID), running_(true) {
    // Prepare the server_addr_.
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port);
    server_addr_.sin_addr.s_addr = addr;

    // Create a socket.
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        throw std::runtime_error("Server Init failed: failed to create a socket.");
    }

    // Set the socket to be reusable.
    int opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Server Init failed: failed to set the socket to be reusable.");
    }

    // Bind the socket to the server address and port.
    if (bind(socketfd, cast_sockaddr_in(server_addr_), sizeof(server_addr_)) < 0) {
        close(socketfd);
        throw std::runtime_error("Server Init failed: failed to bind the socket to the server address and port.");
    }

    // Listen for connections for maximum MAX_CLIENT_NUM clients.
    listen(socketfd, MAX_CLIENT_NUM);

    // Save the socket.
    socketfd_ = socketfd;

    // Create the lists.
    clientinfo_list_ = std::unique_ptr<std::map<uint8_t, std::unique_ptr<ClientInfo> > >(
        new std::map<uint8_t, std::unique_ptr<ClientInfo> >()
    );
    client_thread_list_ = std::unique_ptr<std::map<uint8_t, std::unique_ptr<std::thread> > >(
        new std::map<uint8_t, std::unique_ptr<std::thread> >()
    );
    message_status_map_ = std::unique_ptr<Map<uint16_t, PacketInfo> >(
        new Map<uint16_t, PacketInfo>()
    );
}

Server::~Server() {
    std::cout << "[INFO] Release the server." << std::endl;
    // Close all the client connections.
    for (auto it = clientinfo_list_->begin(); it != clientinfo_list_->end(); it++) {
        // Send a DISCONNECT REQUEST.
        send_res_t result = it->second->get_sender()->send_disconnect_request(it->first);
        // Insert the message into the message_status_map_.
        // Key is DISCONNECT REQUEST's package id, value is the DISCONNECT REQUEST's package info.
        message_status_map_->insert_or_assign(result.first, PacketInfo{
            result.first,
            SERVER_ID,
            it->first,
            MessageType::DISCONNECT
        });
    }

    std::cout << "[INFO] Release the client threads." << std::endl;
    // Join all the client threads.
    join_threads();

    // Close the socket.
    close(socketfd_);
}

uint8_t Server::wait_for_client() {
    // Accept a connection for client.
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socketfd = accept(socketfd_, cast_sockaddr_in(client_addr), &client_addr_len);
    if (!running_) {
        return 0;
    }
    if (client_socketfd < 0) {
        throw std::runtime_error("Server Wait For Client failed: failed to accept a connection.");
    }

    // Receive a CONNECT REQUEST.
    Message request(false);
    Receiver *receiver = new Receiver(client_socketfd, SERVER_ID);
    Sender *sender = new Sender(client_socketfd, SERVER_ID);
    receiver->receive(request);
    // Check if the message is a valid CONNECT REQUEST.
    if (request.get_type() != MessageType::CONNECT ||
        request.get_receiver_id() != SERVER_ID) {
        throw std::runtime_error("Server Wait For Client failed: invalid connection request.");
    }

    // Get the name of the client.
    if (request.get_data().size() != 1) {
        throw std::runtime_error("Server Wait For Client failed: invalid connection request.");
    }
    std::string client_name = request.get_data()[0];

    // Create a client info.
    // Find a valid client id.
    uint8_t id = 1;
    while (clientinfo_list_->find(id) != clientinfo_list_->end()) {
        id++;
    }
    if (id == 0) {
        throw std::runtime_error("Server Wait For Client failed: too many clients.");
    }
    // Create a client info.
    std::unique_ptr<ClientInfo> client_info = std::make_unique<ClientInfo>(
        client_name,
        client_addr,
        client_socketfd,
        id,
        sender,
        receiver
    );
    clientinfo_list_->insert_or_assign(id, std::move(client_info));
    // Create a thread for the client.
    std::unique_ptr<std::thread> client_thread = std::make_unique<std::thread>(
        &Server::receive_from_client,
        this,
        id
    );
    if (client_thread_list_->find(id) != client_thread_list_->end()) {
        client_thread_list_->at(id)->join();
        client_thread_list_->at(id).release();
    }
    client_thread_list_->insert_or_assign(id, std::move(client_thread));

    // Send a CONNECT RESPONSE.
    clientinfo_list_->at(id)->get_sender()->send_acknowledge(request.get_pakage_id(), id);

    return id;
}

void Server::receive_from_client(uint8_t client_id) {
    // Check if the client id is valid.
    if (clientinfo_list_->find(client_id) == clientinfo_list_->end()) {
        throw std::runtime_error("Server Receive From Client failed: invalid client id.");
    }

    Receiver *receiver = clientinfo_list_->at(client_id)->get_receiver();
    Sender *sender = clientinfo_list_->at(client_id)->get_sender();

    Message message;
    std::cout << "[INFO] waiting for message..." << std::endl;
    while (receiver->receive(message)) {
        // Print the message.
        std::cout << "[DEBUG] message: " << message.to_string() << std::endl;
        // check the type of the message
        if (message.get_type () == MessageType::DISCONNECT) {
            // Send an ACK.
            sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
            // Remove the client.
            clientinfo_list_->erase(client_id);
            break;
        } else if (message.get_type() == MessageType::REQSEND) {
            // Try to find the receiver.
            if (clientinfo_list_->find(message.get_receiver_id()) == clientinfo_list_->end()) {
                // Not found.
                data_t data;
                data.push_back("The receiver is not found.");
                std::cerr << "[ERR] The receiver is not found." << std::endl;
                sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id(), data);
                continue;
            }

            // Found, Send a FWD.
            send_res_t result = clientinfo_list_->at(message.get_receiver_id())->get_sender()->send_forward(message);
            // Insert the message into the massage_type_map_.
            // Key is FWD's package id, value is the REQSEND's package info.
            message_status_map_->insert_or_assign(result.first, PacketInfo{
                message.get_pakage_id(),
                message.get_sender_id(),
                message.get_receiver_id(),
                MessageType::FWD
            });
        } else if (message.get_type() == MessageType::ACK) {
            // Check if the message is in the message_status_map_.
            if (message_status_map_->find(message.get_pakage_id()) == message_status_map_->end()) {
                // Not found, do nothing.
                continue;
            }

            // Found, check if the original message is a DISCONNECT REQUEST.
            PacketInfo packet_info = message_status_map_->at(message.get_pakage_id());
            message_status_map_->erase(message.get_pakage_id());
            if (packet_info.message_type == MessageType::DISCONNECT) {
                // DISCONNECT REQUEST, close the thread.
                break;
            }

            // Then check if the message is a FWD.
            // Check if the receiver and sender is swapped.
            if (message.get_sender_id() == packet_info.receiver_id &&
                message.get_receiver_id() == packet_info.sender_id) {
                // Swapped, success, send an ACK to the sender before (the receiver now).
                clientinfo_list_->at(packet_info.sender_id)->get_sender()->send_acknowledge(
                    packet_info.package_id,
                    packet_info.sender_id
                );
            } else {
                // Not swapped, send error message to the sender before.
                data_t data;
                data.push_back("Error in connection between the server and the receiver.");
                std::cerr << "[ERR] " << data[0] << std::endl;
                clientinfo_list_->at(packet_info.sender_id)->get_sender()->send_acknowledge(
                    packet_info.package_id,
                    packet_info.sender_id,
                    data
                );
            }
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
            for (auto it = clientinfo_list_->begin(); it != clientinfo_list_->end(); it++) {
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
            std::time_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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
        std::cout << "Done message: " << message.to_string() << std::endl;
        std::cout << "[INFO] Waiting for message..." << std::endl;
    }
    std::cout << "[INFO] Client " << (int)client_id << " disconnected." << std::endl;
}

void Server::join_threads() {
    for (auto it = client_thread_list_->begin(); it != client_thread_list_->end(); it++) {
        it->second->join();
    }
}

void Server::run() {
    while (running_) {
        try {
            std::cout << "[INFO] Waiting for clients to connect..." << std::endl;
            // Wait for clients to connect.
            uint8_t id = wait_for_client();
            if (!running_) {
                break;
            }
            std::cout << "[INFO] " << clientinfo_list_->at(id)->get_name() << "(ID: " << (int)id << ") connected." << std::endl;
            std::cout << "[INFO] Address: " << inet_ntoa(clientinfo_list_->at(id)->get_addr().sin_addr) << std::endl;
            std::cout << "[INFO] Port: " << ntohs(clientinfo_list_->at(id)->get_addr().sin_port) << std::endl;
        } catch (std::exception &e) {
            std::cerr << "[ERR] " << e.what() << std::endl;
        }
    }
}

void Server::stop() {
    std::cout << "[INFO] Stopping the server..." << std::endl;
    running_ = false;
    shutdown(socketfd_, SHUT_RDWR);
}
