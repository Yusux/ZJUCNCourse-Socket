#include "Server.hpp"
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstring>
#include <netinet/tcp.h>

ClientInfo::ClientInfo(
    std::string name,
    sockaddr_in addr,
    int sockfd,
    uint8_t id,
    Sender *sender,
    Receiver *receiver
) : sockfd_(sockfd), name_(name), addr_(addr), client_id_(id) {
    sender_ = std::unique_ptr<Sender>(sender);
    receiver_ = std::unique_ptr<Receiver>(receiver);
}

ClientInfo::~ClientInfo() {
    // Close the socket.
    close(sockfd_);
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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::string error_msg = "Server Init failed: failed to create a socket. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Set the socket to be reusable.
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(sockfd);
        std::string error_msg = "Server Init failed: failed to set the socket to be reusable. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Bind the socket to the server address and port.
    if (bind(sockfd, cast_sockaddr_in(server_addr_), sizeof(server_addr_)) < 0) {
        close(sockfd);
        std::string error_msg = "Server Init failed: failed to bind the socket to the server address and port. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Listen for connections for maximum MAX_CLIENT_NUM clients.
    listen(sockfd, MAX_CLIENT_NUM);

    // Save the socket.
    sockfd_ = sockfd;

    // Create the lists.
    clientinfo_list_ = std::unique_ptr<Map<uint8_t, std::unique_ptr<ClientInfo> > >(
        new Map<uint8_t, std::unique_ptr<ClientInfo> >()
    );
    client_thread_list_ = std::unique_ptr<Map<uint8_t, std::unique_ptr<std::thread> > >(
        new Map<uint8_t, std::unique_ptr<std::thread> >()
    );
    message_status_map_ = std::unique_ptr<Map<uint16_t, PacketInfo> >(
        new Map<uint16_t, PacketInfo>()
    );
    output_queue_ = std::unique_ptr<Queue<std::string> >(
        new Queue<std::string>()
    );
}

Server::~Server() {
    output_queue_->push("[INFO] Release the server.");
    // Close all the client connections.
    // Get shared_mutex for clientinfo_list_.
    std::unique_lock<std::mutex> lock(clientinfo_list_->get_mutex());
    for (auto it = clientinfo_list_->begin(lock); it != clientinfo_list_->end(lock); it++) {
        // Send a DISCONNECT REQUEST.
        send_res_t result = it->second->get_sender()->send_disconnect_request(it->first);
        // Insert the message into the message_status_map_.
        // Key is DISCONNECT REQUEST's package id, value is the DISCONNECT REQUEST's package info.
        message_status_map_->insert_or_assign(
            result.first,
            PacketInfo {
                result.first,
                SERVER_ID,
                it->first,
                MessageType::DISCONNECT
            },
            lock
        );
    }

    output_queue_->push("[INFO] Release the client threads.");
    // Join all the client threads.
    join_threads();

    // Close the socket.
    close(sockfd_);

    // Output the remaining messages.
    output_message();
}

uint8_t Server::wait_for_client() {
    // Accept a connection for client.
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd_, cast_sockaddr_in(client_addr), &client_addr_len);
    if (!running_) {
        // if the server is not running, close the socket and return.
        if (client_sockfd >= 0) {
            close(client_sockfd);
        }
        return 0;
    }
    if (client_sockfd < 0) {
        std::string error_msg = "Server Wait For Client failed: failed to accept a connection. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Receive a CONNECT REQUEST.
    Message request(false);
    Receiver *receiver = new Receiver(client_sockfd, SERVER_ID);
    Sender *sender = new Sender(client_sockfd, SERVER_ID);
    receiver->receive(request);
    // Check if the message is a valid CONNECT REQUEST.
    if (request.get_type() != MessageType::CONNECT ||
        request.get_receiver_id() != SERVER_ID) {
        close(client_sockfd);
        throw std::runtime_error("Server Wait For Client failed: invalid connection request.");
    }

    // Get the name of the client.
    if (request.get_data().size() != 1) {
        close(client_sockfd);
        throw std::runtime_error("Server Wait For Client failed: invalid connection request.");
    }
    std::string client_name = request.get_data()[0];

    // Create a client info.
    // Find a valid client id.
    uint8_t id = 1;
    std::unique_lock<std::mutex> clientinfo_list_lock(clientinfo_list_->get_mutex());
    while (clientinfo_list_->check_exist(id, clientinfo_list_lock)) {
        id++;
    }
    if (id == 0) {
        close(client_sockfd);
        throw std::runtime_error("Server Wait For Client failed: no free client id.");
    }

    // Set socket to keepalive
    int keepalive = 1;
    int keepidle = 60;
    int keepinterval = 5;
    int keepcount = 3;
    setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
    setsockopt(sockfd_, SOL_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
    setsockopt(sockfd_, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
    setsockopt(sockfd_, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));

    // Create a client info.
    std::unique_ptr<ClientInfo> client_info = std::make_unique<ClientInfo>(
        client_name,
        client_addr,
        client_sockfd,
        id,
        sender,
        receiver
    );
    clientinfo_list_->insert_or_assign(id, std::move(client_info), clientinfo_list_lock);
    // Create a thread for the client.
    std::unique_ptr<std::thread> client_thread = std::make_unique<std::thread>(
        &Server::receive_from_client,
        this,
        id
    );
    std::unique_lock<std::mutex> client_thread_list_lock(client_thread_list_->get_mutex());
    if (client_thread_list_->check_exist(id, client_thread_list_lock)) {
        client_thread_list_->at(id, client_thread_list_lock)->join();
        client_thread_list_->at(id, client_thread_list_lock).release();
    }
    client_thread_list_->insert_or_assign(id, std::move(client_thread), client_thread_list_lock);

    // Send a CONNECT RESPONSE.
    clientinfo_list_->at(id, clientinfo_list_lock)
                    ->get_sender()
                    ->send_acknowledge(request.get_pakage_id(), id);

    return id;
}

void Server::receive_from_client(uint8_t client_id) {
    // Check if the client id is valid.
    std::unique_lock<std::mutex> clientinfo_list_lock(clientinfo_list_->get_mutex());
    if (!clientinfo_list_->check_exist(client_id, clientinfo_list_lock)) {
        throw std::runtime_error("Server Receive From Client failed: invalid client id.");
    }

    Receiver *receiver = clientinfo_list_->at(client_id, clientinfo_list_lock)->get_receiver();
    Sender *sender = clientinfo_list_->at(client_id, clientinfo_list_lock)->get_sender();

    Message message;
    output_queue_->push(
        "[INFO] " + clientinfo_list_->at(client_id, clientinfo_list_lock)->get_name() +
        "(ID: " + std::to_string(client_id) + ") connected."
    );
    output_queue_->push(
        "[INFO] Address: " +
        std::string(
            inet_ntoa(clientinfo_list_->at(client_id, clientinfo_list_lock)->get_addr().sin_addr)
        )
    );
    output_queue_->push(
        "[INFO] Port: " +
        std::to_string(
            ntohs(clientinfo_list_->at(client_id, clientinfo_list_lock)->get_addr().sin_port)
        )
    );
    output_queue_->push("[INFO] waiting for message...");
    // delete the unique_lock
    clientinfo_list_lock.unlock();
    while (receiver->receive(message)) {
        std::unique_lock<std::mutex> lock(clientinfo_list_->get_mutex());
        // Check if the message is from the client.
        if (message.get_sender_id() != client_id) {
            // Not from the client, do nothing.
            continue;
        }
        // Print the message.
        output_queue_->push("[DEBUG] Received message: " + message.to_string());
        // check the type of the message
        if (message.get_type () == MessageType::DISCONNECT) {
            // Send an ACK.
            sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
            break;
        } else if (message.get_type() == MessageType::REQSEND) {
            // Try to find the receiver.
            if (!clientinfo_list_->check_exist(message.get_receiver_id(), lock)) {
                // Not found.
                data_t data;
                data.push_back("The receiver is not found.");
                output_queue_->push("[ERR] The receiver is not found.");
                sender->send_acknowledge(message.get_pakage_id(), message.get_sender_id(), data);
                continue;
            }

            // Found, Send a FWD.
            send_res_t result = clientinfo_list_->at(message.get_receiver_id(), lock)
                                                ->get_sender()
                                                ->send_forward(message);
            // Insert the message into the massage_type_map_.
            // Key is FWD's package id, value is the REQSEND's package info.
            message_status_map_->insert_or_assign(
                result.first,
                PacketInfo{
                    message.get_pakage_id(),
                    message.get_sender_id(),
                    message.get_receiver_id(),
                    MessageType::FWD
                },
                lock
            );
        } else if (message.get_type() == MessageType::ACK) {
            // Check if the message is in the message_status_map_.
            if (!message_status_map_->check_exist(message.get_pakage_id(), lock)) {
                // Not found, do nothing.
                continue;
            }

            // Found, check if the original message is a DISCONNECT REQUEST.
            PacketInfo packet_info = message_status_map_->at(message.get_pakage_id(), lock);
            message_status_map_->erase(message.get_pakage_id(), lock);
            if (packet_info.message_type == MessageType::DISCONNECT) {
                // DISCONNECT REQUEST, close the thread.
                break;
            }

            // Then check if the message is a FWD.
            // Check if the receiver and sender is swapped.
            if (message.get_sender_id() == packet_info.receiver_id &&
                message.get_receiver_id() == packet_info.sender_id) {
                // Swapped, success, send an ACK to the sender before (the receiver now).
                clientinfo_list_->at(packet_info.sender_id, lock)->get_sender()->send_acknowledge(
                    packet_info.package_id,
                    packet_info.sender_id
                );
            } else {
                // Not swapped, send error message to the sender before.
                data_t data;
                data.push_back("Error in connection between the server and the receiver.");
                output_queue_->push("[ERR] " + data[0]);
                clientinfo_list_->at(packet_info.sender_id, lock)->get_sender()->send_acknowledge(
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
             */
            for (auto it = clientinfo_list_->begin(lock); it != clientinfo_list_->end(lock); it++) {
                std::string id_str = std::to_string(it->first);
                std::string name_str = it->second->get_name();
                std::string ip_str = inet_ntoa(it->second->get_addr().sin_addr);
                std::string port_str = std::to_string(ntohs(it->second->get_addr().sin_port));
                std::string client_str = id_str + DIVISION_SIGNAL +
                                         name_str + DIVISION_SIGNAL +
                                         ip_str + DIVISION_SIGNAL +
                                         port_str + DIVISION_SIGNAL;
                data.push_back(client_str);
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
        output_queue_->push("[DEBUG] Done message: " + message.to_string());
        output_queue_->push("[INFO] Waiting for message...");
    }
    // Relock the unique_lock
    clientinfo_list_lock.lock();
    output_queue_->push(
        "[INFO] " + clientinfo_list_->at(client_id, clientinfo_list_lock)->get_name() +
        "(ID: " + std::to_string(client_id) + ") disconnected."
    );
    // Remove the client.
    clear_message_status_map(client_id);
    clientinfo_list_->erase(client_id, clientinfo_list_lock);
}

void Server::join_threads() {
    std::unique_lock<std::mutex> lock(client_thread_list_->get_mutex());
    for (auto it = client_thread_list_->begin(lock); it != client_thread_list_->end(lock); it++) {
        it->second->join();
    }
}

void Server::run() {
    while (running_) {
        try {
            // Wait for clients to connect.
            uint8_t id = wait_for_client();
            if (!running_) {
                break;
            }
        } catch (std::exception &e) {
            output_queue_->push("[ERR] " + std::string(e.what()));
        }
    }
}

void Server::stop() {
    output_queue_->push("[INFO] Stopping the server...");
    running_ = false;
    shutdown(sockfd_, SHUT_RDWR);
}

bool Server::output_message() {
    if (output_queue_->empty()) {
        return false;
    }
    std::cout << std::endl;
    while (!output_queue_->empty()) {
        std::string output = output_queue_->pop();
        std::cout << output << std::endl;
    }
    return true;
}

bool Server::clear_message_status_map(uint16_t client_id) {
    std::unique_lock<std::mutex> lock(message_status_map_->get_mutex());
    std::unique_lock<std::mutex> clientinfo_list_lock(clientinfo_list_->get_mutex());
    for (auto it = message_status_map_->begin(lock); it != message_status_map_->end(lock);) {
        if (it->second.sender_id == client_id) {
            // Erase the message.
            it = message_status_map_->erase(it, lock);
        } else if (it->second.receiver_id == client_id) {
            // check if the sender is still connected.
            if (!clientinfo_list_->check_exist(it->second.sender_id, clientinfo_list_lock)) {
                // Not found, do nothing.
                it++;
                continue;
            }
            // Send an ACK to the sender with error message.
            data_t data;
            data.push_back("Error in connection because the receiver is disconnected.");
            clientinfo_list_->at(it->second.sender_id, clientinfo_list_lock)
                            ->get_sender()
                            ->send_acknowledge(
                                it->second.package_id,
                                it->second.sender_id,
                                data
                            );
            // Erase the message.
            it = message_status_map_->erase(it, lock);
        } else {
            it++;
        }
    }
    return true;
}
