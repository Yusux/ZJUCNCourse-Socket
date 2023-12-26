#include "Client.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    // Prepare arguments.
    char *hostname = new char[128];
    gethostname(hostname, sizeof(hostname));
    std::string name(hostname);
    delete hostname;
    in_addr_t addr = SERVER_ADDR;
    int port = SERVER_PORT;

    // If there are arguments, use them.
    // in order: <name> <addr> <port>
    if (argc > 1) {
        name = argv[1];
    }
    if (argc > 2) {
        addr = inet_addr(argv[2]);
    }
    if (argc > 3) {
        port = atoi(argv[3]);
    }

    std::cout << "Client name: " << name << std::endl;
    std::cout << "Server address: " << inet_ntoa(*(in_addr *)&addr) << std::endl;
    std::cout << "Server port: " << port << std::endl;

    // Create a client.
    Client client(name);

    /*
     * 1. Connect to the server.
     * 2. Disconnect from the server.
     * 3. Get the time from the server.
     * 4. Get the name of the server.
     * 5. Get the list of the clients.
     * 6. Send a message to a client.
     * 0. Exit.
     */

    int choice = -1;
    while (true) {
        std::cin >> choice;
        try {
            switch (choice) {
                case 1: {
                    // Connect to the server.
                    if (client.connect_to_server(addr, port)) {
                        std::cout << "Connected to the server." << std::endl;
                    } else {
                        std::cout << "Failed to connect to the server." << std::endl;
                    }
                    break;
                }
                case 2: {
                    // Disconnect from the server.
                    if (client.disconnect_from_server()) {
                        std::cout << "Disconnected from the server." << std::endl;
                    } else {
                        std::cout << "Failed to disconnect from the server." << std::endl;
                    }
                    break;
                }
                case 3: {
                    // Get the time from the server.
                    std::string time = client.get_time();
                    std::cout << "Time: " << time << std::endl;
                    break;
                }
                case 4: {
                    // Get the name of the server.
                    std::string name = client.get_name();
                    std::cout << "Server name: " << name << std::endl;
                    break;
                }
                case 5: {
                    // Get the list of the clients.
                    data_t client_list = client.get_client_list();
                    std::cout << "Client list:" << std::endl;
                    for (int i = 0; i < client_list.size(); i += 5) {
                        std::cout << "ID: " << client_list[i] << std::endl;
                        std::cout << "Name: " << client_list[i + 1] << std::endl;
                        std::cout << "Address: " << client_list[i + 2] << std::endl;
                        std::cout << "Port: " << client_list[i + 3] << std::endl;
                        std::cout << std::endl;
                    }
                    break;
                }
                case 6: {
                    // Send a message to a client.
                    int id_read;
                    std::string content;
                    std::cin >> id_read >> content;
                    // ensure that the id is valid.
                    if (id_read < 0 || id_read > 255) {
                        throw std::runtime_error("Invalid client id.");
                    }
                    // transform the id to unsigned char.
                    unsigned char receiver_id = (unsigned char)id_read;
                    std::cout << "Sending message \"" << content << "\" to client " << (int)receiver_id << std::endl;
                    if (client.send_message(receiver_id, content)) {
                        std::cout << "Message sent." << std::endl;
                    } else {
                        std::cout << "Failed to send message." << std::endl;
                    }
                    break;
                }
                case 7: {
                    // Keep receiving messages from the server.
                    client.receive_message();
                    break;
                }
                case 0: {
                    // Exit.
                    return 0;
                }
                default: {
                    std::cout << "Invalid choice." << std::endl;
                    break;
                }
            }
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}