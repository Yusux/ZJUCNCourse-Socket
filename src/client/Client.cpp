#include "Client.hpp"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cstring>

Client::Client(
    std::string name
) : name_(name) {  // in order not to increase the pakage id
    // Prepare the server_addr_.
    server_addr_.sin_family = AF_INET;

    // Not to create a socket until call connect_to_server().
    socketfd_ = -1;

    // Not to connect to the server until call connect_to_server().
    sender_.release();
    receiver_.release();

    // Not to get self_id_ until call connect_to_server().
    self_id_ = 0;

    // Initialize the message_type_map_.
    message_type_map_ = std::make_unique<Map<uint16_t, MessageType> >();

    // Initialize the message_queue_.
    output_queue_ = std::make_unique<Queue<std::string> >();
}

Client::~Client() {
    if (socketfd_ < 0) {
        // Not connected to the server.
        join_threads();
        return;
    }

    // Try to disconnect from the server.
    try {
        disconnect_from_server();
        // Wait for the threads to finish.
        join_threads();
    } catch (const std::exception &e) {
        // Error in disconnection, delete socketfd_ anyway.
        output_queue_->push("[WARN] " + std::string(e.what()));
        output_queue_->push("[WARN] Release the client anyway.");
        // Close the socket.
        close(socketfd_);
        // Terminate the threads.
        std::thread::native_handle_type handle;
        if (receive_thread_->joinable()) {
            handle = receive_thread_->native_handle();
            pthread_cancel(handle);
        }
    }
}

bool Client::connect_to_server(in_addr_t addr, int port) {
    if (socketfd_ >= 0) {
        throw std::runtime_error("Connect Request failed: already connected to the server.");
    }
    // Prepare the server_addr_.
    server_addr_.sin_port = htons(port);
    server_addr_.sin_addr.s_addr = addr;

    // Create a socket.
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        std::string error_msg = "Connect Request failed: failed to create a socket. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Connect to the server.
    if (connect(socketfd, cast_sockaddr_in(server_addr_), sizeof(server_addr_)) < 0) {
        close(socketfd);
        std::string error_msg = "Connect Request failed: failed to connect to the server. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Create a sender and a receiver.
    sender_ = std::make_unique<Sender>(socketfd, 0);
    receiver_ = std::make_unique<Receiver>(socketfd, 0);

    // Send a Connect Request.
    send_res_t result = sender_->send_connect_request(name_);
    Message response(false);
    receiver_->receive(response);
    if (check_afk(response, result)) {
        // Successfully connected to the server.
        self_id_ = response.get_receiver_id();
        sender_->set_self_id(self_id_);
        receiver_->set_self_id(self_id_);
        // Start the threads.
        join_threads();
        message_type_map_->clear();
        receive_thread_ = std::move(
            std::make_unique<std::thread>(
                std::thread(
                    &Client::receive_message,
                    this
                )
            )
        );
        output_queue_->push("[INFO] Connected to the server with name \"" + name_ + "\" and id \"" + std::to_string((int)self_id_) + "\".");
    } else {
        // Error in connection.
        close(socketfd);
        throw std::runtime_error("Connect Request failed: error in connection.");
    }

    // Successfully connected to the server.
    socketfd_ = socketfd;
    return true;
}

bool Client::disconnect_from_server() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("Request failed: not connected to the server.");
    }

    // Send a Disconnect Request.
    send_res_t result = sender_->send_disconnect_request();
    if (message_type_map_->find(result.first) != message_type_map_->end()) {
        throw std::runtime_error("Disconnect Request failed: pakage id already exists.");
    }
    message_type_map_->insert_or_assign(result.first, MessageType::DISCONNECT);

    return true;
}

bool Client::get_time() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("Request failed: not connected to the server.");
    }

    // Send a Request Time.
    send_res_t result = sender_->send_request_time();
    if (message_type_map_->find(result.first) != message_type_map_->end()) {
        throw std::runtime_error("Request Time failed: pakage id already exists.");
    }
    message_type_map_->insert_or_assign(result.first, MessageType::REQTIME);

    return true;
}

bool Client::get_name() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("Request failed: not connected to the server.");
    }

    // Send a Request HOST.
    send_res_t result = sender_->send_request_host();
    if (message_type_map_->find(result.first) != message_type_map_->end()) {
        throw std::runtime_error("Request HOST failed: pakage id already exists.");
    }
    message_type_map_->insert_or_assign(result.first, MessageType::REQHOST);

    return true;
}

bool Client::get_client_list() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("Request failed: not connected to the server.");
    }

    // Send a Request Client List.
    send_res_t result = sender_->send_request_client_list();
    if (message_type_map_->find(result.first) != message_type_map_->end()) {
        throw std::runtime_error("Request Client List failed: pakage id already exists.");
    }
    message_type_map_->insert_or_assign(result.first, MessageType::REQCLILIST);

    return true;
}

bool Client::send_message(uint8_t receiver_id, std::string content) {
    static int cnt = 0;
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("Request failed: not connected to the server.");
    }

    // Send a Request Send.
    output_queue_->push("[DEBUG] Send message No." + std::to_string(++cnt));
    send_res_t result = sender_->send_request_send(receiver_id, content);
    if (message_type_map_->find(result.first) != message_type_map_->end()) {
        throw std::runtime_error("Request Send failed: pakage id already exists.");
    }
    message_type_map_->insert_or_assign(result.first, MessageType::REQSEND);

    return true;
}

void Client::receive_message() {
    // Check if connected to the server.
    if (socketfd_ < 0) {
        throw std::runtime_error("Disconnect Request failed: not connected to the server.");
    }

    Message message;
    int cnt = 0;
    while (receiver_->receive(message)) {
        output_queue_->push("[DEBUG] Receive message No." +
                            std::to_string(++cnt) +
                            " : " +
                            message.to_string());
        // Check the type of the message
        if (message.get_type() == MessageType::DISCONNECT) {
            // Send an ACK.
            sender_->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
            break;
        } else if (message.get_type() == MessageType::FWD) {
            std::string content;
            data_t data = message.get_data();
            for (auto &str : data) {
                content += str;
                content += "$\n";
            }
            output_queue_->push("Message from client " +
                                std::to_string((int)message.get_sender_id()) +
                                ": " +
                                content);
            // Send an ACK.
            sender_->send_acknowledge(message.get_pakage_id(), message.get_sender_id());
        } else if (message.get_type() == MessageType::ACK) {
            // Check if the pakage id exists.
            if (message_type_map_->find(message.get_pakage_id()) == message_type_map_->end()) {
                // ignore the message.
                continue;
            }

            // Get the type of the message.
            MessageType type = message_type_map_->at(message.get_pakage_id());
            // Remove the pakage id.
            message_type_map_->erase(message.get_pakage_id());
            // Check if the sender id is correct.
            if (message.get_sender_id() != SERVER_ID) {
                output_queue_->push("[ERR] ACK failed: wrong sender id.");
                if (type != MessageType::DISCONNECT) {
                    // ignore the message.
                    continue;
                } else {
                    output_queue_->push("[WARN] Release the connection anyway.");
                }
            }

            // Do different things according to the type of the message.
            if (type == MessageType::DISCONNECT) {
                break;
            } else if (type == MessageType::REQTIME) {
                // Get the time.
                data_t data = message.get_data();
                if (data.size() != 1) {
                    output_queue_->push("[ERR] Request Time failed: data size is not 1.");
                    // ignore the message.
                    continue;
                }
                std::string timestamp = data[0];
                std::time_t t = std::stol(timestamp);
                std::string time = std::ctime(&t);
                // remove the '\n' at the end of the string.
                time.pop_back();
                output_queue_->push("Time: " + time);
            } else if (type == MessageType::REQHOST) {
                // Get the name.
                data_t data = message.get_data();
                if (data.size() != 1) {
                    output_queue_->push("[ERR] Request Host failed: data size is not 1.");
                    // ignore the message.
                    continue;
                }
                output_queue_->push("Server name: " + data[0]);
            } else if (type == MessageType::REQCLILIST) {
                // Get the client list.
                data_t data = message.get_data();
                /* In the data, one client occupies 4 elements:
                 * 1. id
                 * 2. name
                 * 3. ip
                 * 4. port
                 */
                output_queue_->push("---- Client List ----");
                for (auto it : data) {
                    // Find the positions of the 4 DIVISION_SIGNALs
                    int pos1 = it.find(DIVISION_SIGNAL);
                    int pos2 = it.find(DIVISION_SIGNAL, pos1 + 1);
                    int pos3 = it.find(DIVISION_SIGNAL, pos2 + 1);
                    int pos4 = it.find(DIVISION_SIGNAL, pos3 + 1);
                    if (pos1 == std::string::npos ||
                        pos2 == std::string::npos ||
                        pos3 == std::string::npos ||
                        pos4 == std::string::npos) {
                        output_queue_->push("[ERR] Request Client List failed: invalid data.");
                        // ignore the message.
                        continue;
                    }
                    std::string id_str = it.substr(0, pos1);
                    std::string name_str = it.substr(pos1 + 1, pos2 - pos1 - 1);
                    std::string ip_str = it.substr(pos2 + 1, pos3 - pos2 - 1);
                    std::string port_str = it.substr(pos3 + 1, pos4 - pos3 - 1);
                    output_queue_->push("  ID: " + id_str);
                    output_queue_->push("Name: " + name_str);
                    output_queue_->push("  IP: " + ip_str);
                    output_queue_->push("Port: " + port_str);
                    output_queue_->push("---------------------");
                }
            } else if (type == MessageType::REQSEND) {
                // Get the result.
                data_t data = message.get_data();
                if (data.size() != 0) {
                    std::string error_msg = "[ERR] Request Send failed: " + data[0];
                    output_queue_->push(error_msg);
                } else {
                    output_queue_->push("[INFO] Request Send succeeded.");
                }
            } else {
                output_queue_->push("[ERR] Unknown message type.");
            }
        } else {
            output_queue_->push("[ERR] Unknown message type.");
        }
    }
    // Remove the connection.
    close(socketfd_);
    socketfd_ = -1;
    sender_.release();
    receiver_.release();
    output_queue_->push("[INFO] Disconnected from the server.");
}

void Client::join_threads() {
    if (receive_thread_ != nullptr && receive_thread_->joinable()) {
        receive_thread_->join();
        receive_thread_.release();
    }
}

bool Client::output_message() {
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
