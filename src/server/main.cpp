#include "Server.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    // Prepare arguments.
    char *hostname = new char[128];
    gethostname(hostname, sizeof(hostname));
    std::string name(hostname);
    delete[] hostname;
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

    // Create a server.
    Server server(name, addr, port);

    // Wait for clients to connect.
    unsigned char id = server.wait_for_client();
    std::cout << server.get_client_name(id) << "(ID: " << (int)id << ") connected." << std::endl;
    std::cout << "Address: " << inet_ntoa(server.get_client_addr(id).sin_addr) << std::endl;
    std::cout << "Port: " << ntohs(server.get_client_addr(id).sin_port) << std::endl;
    // Wait for clients to connect.
    id = server.wait_for_client();
    std::cout << server.get_client_name(id) << "(ID: " << (int)id << ") connected." << std::endl;
    std::cout << "Address: " << inet_ntoa(server.get_client_addr(id).sin_addr) << std::endl;
    std::cout << "Port: " << ntohs(server.get_client_addr(id).sin_port) << std::endl;

    // keep receiving messages from the client.
    server.receive_from_client(id);
}