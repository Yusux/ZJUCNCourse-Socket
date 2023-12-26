#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "def.hpp"
#include "Message.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include <map>

class ClientInfo {
private:
    int socketfd_;
    std::string name_;
    sockaddr_in addr_;
    unsigned char client_id_;
    std::unique_ptr<Sender> sender_;
    std::unique_ptr<Receiver> receiver_;

public:
    ClientInfo(std::string name,
               sockaddr_in addr,
               int socketfd,
               unsigned char id,
               Sender *sender,
               Receiver *receiver);
    ~ClientInfo();

    std::string get_name();
    sockaddr_in get_addr();
    Sender *get_sender();
    Receiver *get_receiver();
};

class Server {
private:
    int socketfd_;
    const std::string name_;
    sockaddr_in server_addr_;
    unsigned char self_id_;
    Message last_msg_;
    client_map_t client_list_;

public:
    /*
     * Connect to the server.
     * @param name The name of the client.
     */
    Server(std::string name, in_addr_t addr, int port);
    ~Server();

    /*
     * Wait for clients to connect.
     * @return a valid client id.
     */
    unsigned char wait_for_client();

    /*
     * Keep receiving messages from the client.
     * @param client_id The id of the client.
     * @param message The message to be received.
     */
    void receive_from_client(unsigned char client_id);

    std::string get_client_name(unsigned char client_id);
    sockaddr_in get_client_addr(unsigned char client_id);
};

#endif
