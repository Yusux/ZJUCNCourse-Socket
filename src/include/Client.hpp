#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include "def.hpp"
#include "Message.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"
#include "Map.hpp"
#include "Queue.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

class Client {
private:
    int sockfd_;
    const std::string name_;
    sockaddr_in server_addr_;
    uint8_t self_id_;

    std::unique_ptr<std::thread> receive_thread_;

    std::unique_ptr<Sender> sender_;
    std::unique_ptr<Receiver> receiver_;
    std::unique_ptr<Map<uint16_t, MessageType> > message_type_map_;
    std::unique_ptr<Queue<std::string> > output_queue_;

    /*
     * Keep receiving messages from the server.
     */
    void receive_message();

    /*
     * Join the threads.
     */
    void join_threads();

    /*
     * Check whether the message exists.
     * @param result The result of sending the message.
     * @param lock The unique_lock of the mutex.
     * @return Whether the message exists.
     */
    bool check_message_exist(send_res_t result, std::unique_lock<std::mutex> &lock);

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
    bool get_time();

    /*
     * Get the name of the server.
     * @return The name of the server.
     */
    bool get_name();

    /*
     * Get the list of the clients.
     * @return The list of the clients.
     */
    bool get_client_list();

    /*
     * Send a message to the server.
     * @param receiver_id The id of the receiver.
     * @param content The content of the message.
     * @return Whether the sending is successful.
     */
    bool send_message(uint8_t receiver_id, std::string content);

    /*
     * Print the message queue.
     * @return Whether the printing is successful.
     */
    bool output_message();
};

#endif
