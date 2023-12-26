#ifndef __SENDER_HPP__
#define __SENDER_HPP__

#include "def.hpp"
#include "Message.hpp"

class Sender {
private:
    int socketfd_;
    unsigned char self_id_;
    std::vector<unsigned char> buffer_;

public:
    /*
     * Constructor.
     * @param socket: The socket to send messages on.
     * @param self_id: The id of the sender.
     */
    explicit Sender(int socket, unsigned char self_id);
    /*
     * Destructor.
     */
    ~Sender();

    /*
     * Change self_id_.
     * @param self_id: The new self_id.
     */
    void set_self_id(unsigned char self_id);

    // FOR CLIENTS ONLY
    /*
     * Send a CONNECT REQUEST packet.
     * @param name: The name of the client.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_connect_request(std::string name);

    /*
     * Send a DISCONNECT REQUEST packet.
     * @param receiver_id: The id of the receiver.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_disconnect_request(unsigned char receiver_id = SERVER_ID);

    /*
     * Send a REQUEST TIME packet.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_request_time();

    /*
     * Send a REQUEST HOST packet.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_request_host();

    /*
     * Send a REQUEST CLIENT LIST packet.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_request_client_list();

    /*
     * Send a REQUEST SEND packet.
     * @param receiver_id: The id of the receiver.
     * @param msg_string: The message to send.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_request_send(unsigned char receiver_id,
                                                       std::string msg_string);

    // FOR SERVER AND CLIENTS
    /*
     * Send an ACKNOWLEDGE packet.
     * @param pakage_id: The id of the packet to acknowledge.
     * @param receiver_id: The id of the receiver.
     * @param data: The data to send with the acknowledgement.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_acknowledge(unsigned char pakage_id,
                        unsigned char receiver_id,
                        const data_t &data = {});

    /*
     * Send a FORWARD packet.
     * Packet id is set to the next packet id according to the counter.
     * @param message: The message to forward.
     * @return: The message id and the number of bytes sent.
     */
    send_res_t send_forward(Message message);
};

#endif