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

    std::cout << "[INFO] Server host name: " << name << std::endl;
    std::cout << "[INFO] Server address: " << inet_ntoa(*(in_addr *)&addr) << std::endl;
    std::cout << "[INFO] Server port: " << port << std::endl;

    // Create a server.
    std::unique_ptr<Server> server;
    try {
        server = std::unique_ptr<Server>(new Server(name, addr, port));
    } catch (std::exception &e) {
        std::cout << "[ERR] " << e.what() << std::endl;
        return 1;
    }

    std::thread runner(&Server::run, server.get());

    std::string command;
    while (true) {
        std::cin >> command;
        if (command == "exit") {
            server->stop();
            runner.join();
            break;
        } else {
            std::cout << "[INFO] Please enter \"exit\" to close the server." << std::endl;
        }
    }
}