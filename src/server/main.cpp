#include "Server.hpp"
#include <iostream>
#include <future>

std::string get_command() {
    std::string command;
    std::getline(std::cin, command);
    return command;
}

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

    // Create a thread to run the server.
    std::thread runner(&Server::run, server.get());
    // Create a thread to get commands.
    std::future<std::string> command_future = std::async(std::launch::async, get_command);

    std::string command;
    std::future_status status;
    try {
        while (true) {
            // Print outputs in the msg queue.
            server->output_message();

            // Get the command.
            if (status == std::future_status::deferred) {
                command_future = std::async(std::launch::async, get_command);
            }
            status = command_future.wait_for(std::chrono::milliseconds(1));
            if (status == std::future_status::ready) {
                command = command_future.get();
                // Prepare but not start the next thread.
                status = std::future_status::deferred;
            } else if (status == std::future_status::timeout) {
                continue;
            } else { // status == std::future_status::deferred
                throw std::runtime_error("Invalid future status (Deferred).");
            }

            if (command == "exit") {
                break;
            } else {
                std::cout << "[INFO] Please enter \"exit\" to close the server." << std::endl;
            }
        }
    } catch (std::exception &e) {
        // Stop the server.
        server->stop();
        runner.join();
        std::cerr << "[ERR] " << e.what() << std::endl;
        return 1;
    }

    // Stop the server.
    server->stop();
    runner.join();

    return 0;
}