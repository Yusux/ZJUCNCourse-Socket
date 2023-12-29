#ifndef __SERVER_HPP__
#define __SERVER_HPP__

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

class ClientInfo {
private:
    int sockfd_;
    std::string name_;
    sockaddr_in addr_;
    uint8_t client_id_;
    std::unique_ptr<Sender> sender_;
    std::unique_ptr<Receiver> receiver_;
    std::unique_ptr<std::map<uint16_t, MessageType> > message_type_map_;


public:
    ClientInfo(std::string name,
               sockaddr_in addr,
               int sockfd,
               uint8_t id,
               Sender *sender,
               Receiver *receiver);
    ~ClientInfo();

    std::string get_name();
    sockaddr_in get_addr();
    Sender *get_sender();
    Receiver *get_receiver();
};

struct PacketInfo {
    uint16_t package_id;
    uint8_t sender_id;
    uint8_t receiver_id;
    MessageType message_type;
};

class Server {
private:
    int sockfd_;
    const std::string name_;
    sockaddr_in server_addr_;
    uint8_t self_id_;
    bool running_;
    // Use Map/Queue with mutex for thread safety.
    std::unique_ptr<Map<uint8_t, std::unique_ptr<ClientInfo> > > clientinfo_list_;
    std::unique_ptr<Map<uint8_t, std::unique_ptr<std::thread> > > client_thread_list_;
    std::unique_ptr<Map<uint16_t, PacketInfo> > message_status_map_;
    std::unique_ptr<Queue<std::string> > output_queue_;

    /*
     * Wait for clients to connect.
     * @return a valid client id.
     */
    uint8_t wait_for_client();

    /*
     * Keep receiving messages from the client.
     * @param client_id The id of the client.
     * @param message The message to be received.
     */
    void receive_from_client(uint8_t client_id);

    /*
     * Join the threads.
     */
    void join_threads();

    /*
     * Clear messages to process in the message_status_map_
     * with the given client id.
     * @param client_id The id of the client.
     * @return Whether the clearing is successful.
     */
    bool clear_message_status_map(uint16_t client_id);

public:
    /*
     * Connect to the server.
     * @param name The name of the client.
     */
    Server(std::string name, in_addr_t addr, int port);
    ~Server();

    /*
     * Run the server.
     * Keeping accepting connections from clients.
     * Once a client is connected, a thread will be created for the client
     * to receive messages from the client and do the corresponding actions.
     */
    void run();

    /*
     * Stop the server.
     */
    void stop();

    /*
     * Print the message queue.
     * @return Whether the printing is successful.
     */
    bool output_message();
};

#endif
