#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include "def.hpp"
#include "Message.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>

class Client {
private:
    int socketfd_;
    const std::string name_;
    sockaddr_in server_addr_;
    unsigned char self_id_;
    std::unique_ptr<Sender> sender_;
    std::unique_ptr<Receiver> receiver_;
    Message last_msg_;

public:
    /*
     * Connect to the server.
     * @param name The name of the client.
     */
    Client(std::string name);
    ~Client();

    /*
     * Connect to the server.
     * @param addr The address of the server.
     * @param port The port of the server.
     * @return Whether the connection is successful.
     */
    bool connect_to_server(in_addr_t addr, int port);

    /*
     * Disconnect from the server.
     * @return Whether the disconnection is successful.
     */
    bool disconnect_from_server();

    /*
     * Get the time from the server.
     * @return The time from the server.
     */
    std::string get_time();

    /*
     * Get the name of the server.
     * @return The name of the server.
     */
    std::string get_name();

    /*
     * Get the list of the clients.
     * @return The list of the clients.
     */
    data_t get_client_list();

    /*
     * Send a message to the server.
     * @param receiver_id The id of the receiver.
     * @param content The content of the message.
     * @return Whether the sending is successful.
     */
    bool send_message(unsigned char receiver_id, std::string content);

    /*
     * Keep receiving messages from the server.
     * @param message The message received.
     */
    void receive_message();

    // Gets
    std::string get_name() const;
    unsigned char get_self_id() const;
    Message get_last_msg() const;
};

#endif
