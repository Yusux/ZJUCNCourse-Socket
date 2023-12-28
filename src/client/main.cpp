#include "Client.hpp"
#include <iostream>
#include <future>
#include <chrono>

enum Choice {
    BAD_CHOICE = -1,
    EXIT = 0,
    CONNECT = 1,
    DISCONNECT,
    GET_TIME,
    GET_NAME,
    GET_CLIENT_LIST,
    SEND_MESSAGE,
    HELP
};

Choice get_choice(std::string choice) {
    if (choice == "exit") {
        return EXIT;
    } else if (choice == "connect") {
        return CONNECT;
    } else if (choice == "disconnect") {
        return DISCONNECT;
    } else if (choice == "gettime") {
        return GET_TIME;
    } else if (choice == "gethost") {
        return GET_NAME;
    } else if (choice == "getcli") {
        return GET_CLIENT_LIST;
    } else if (choice == "send") {
        return SEND_MESSAGE;
    } else if (choice == "help") {
        return HELP;
    } else {
        return BAD_CHOICE;
    }
}

void print_help() {
    std::cout << "1. connect [<server address> <server port>]: Connect to the server." << std::endl
                << "\t<server address>: The address of the server." << std::endl
                << "\t<server port>: The port of the server." << std::endl
                << "\tIf not specified, the default address and port will be used." << std::endl
                << "2. disconnect: Disconnect from the server." << std::endl
                << "3. gettime: Get the time from the server." << std::endl
                << "4. gethost: Get the name of the server." << std::endl
                << "5. getcli: Get the list of the clients." << std::endl
                << "6. send <id> \"<content>\": Send a message to a client." << std::endl
                << "\t<id>: The id of the receiver." << std::endl
                << "\t<content>: The content of the message. Need to be quoted." << std::endl
                << "7. help: Print this message." << std::endl
                << "0. exit: Exit." << std::endl
                << std::endl;
}

std::string get_command() {
    std::cout << "client> ";
    std::string command;
    std::getline(std::cin, command);
    return command;
}

bool process_command(std::string command,
                     std::shared_ptr<Client> client,
                     in_addr_t addr,
                     int port) {
    std::string choice = command.substr(0, command.find(' '));
    switch (get_choice(choice)) {
        case Choice::CONNECT : {
            in_addr_t connect_addr = addr;
            int connect_port = port;
            int pos1 = command.find(' ');
            int pos2 = command.find(' ', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                std::string connect_addr_str = command.substr(pos1 + 1, pos2 - pos1 - 1);
                std::string connect_port_str = command.substr(pos2 + 1);
                connect_addr = inet_addr(connect_addr_str.c_str());
                connect_port = atoi(connect_port_str.c_str());
            }
            // Connect to the server.
            client->connect_to_server(addr, port);
            break;
        }
        case Choice::DISCONNECT : {
            // Disconnect from the server.
            client->disconnect_from_server();
            break;
        }
        case Choice::GET_TIME : {
            // Get the time from the server.
            client->get_time();
            break;
        }
        case Choice::GET_NAME : {
            // Get the name of the server.
            client->get_name();
            break;
        }
        case Choice::GET_CLIENT_LIST : {
            // Get the list of the clients.
            client->get_client_list();
            break;
        }
        case Choice::SEND_MESSAGE : {
            int pos1 = command.find(' ');
            int pos2 = command.find('\"', pos1 + 1);
            int pos3 = command.find('\"', pos2 + 1);
            if (pos1 == std::string::npos ||
                pos2 == std::string::npos ||
                pos3 == std::string::npos) {
                std::cerr << "[WARN] Invalid input: " << command << std::endl;
                break;
            }
            // Get the id and the content.
            std::string id_str = command.substr(pos1 + 1, pos2 - pos1 - 2);
            std::string content = command.substr(pos2 + 1, pos3 - pos2 - 1);
            int id = atoi(id_str.c_str());
            if (id < 0 || id > 255) {
                std::cerr << "[WARN] Invalid id." << std::endl;
                break;
            }
            // transform the id to uint8_t.
            uint8_t receiver_id = (uint8_t)id;
            std::cout << "[INFO] Sending message \"" << content
                      << "\" to client " << (int)receiver_id << std::endl;
            // for (int i = 0; i < 100; i++) {
                client->send_message(receiver_id, content);
            // }
            break;
        }
        case Choice::HELP : {
            // Help.
            print_help();
            break;
        }
        case Choice::EXIT : {
            // Exit.
            return 0;
        }
        default: {
            std::cerr << "[WARN] Invalid choice." << std::endl;
            break;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    // Prepare arguments.
    char *hostname = new char[128];
    gethostname(hostname, sizeof(hostname));
    std::string name(hostname);
    delete hostname;
    in_addr_t addr = SERVER_ADDR;
    int port = SERVER_PORT;

    if (argc > 1) {
        name = argv[1];
    }
    if (argc > 2) {
        addr = inet_addr(argv[2]);
    }
    if (argc > 3) {
        port = atoi(argv[3]);
    }

    std::cout << "[INFO] Client name: " << name << std::endl;
    std::cout << "[INFO] Server address: " << inet_ntoa(*(in_addr *)&addr) << std::endl;
    std::cout << "[INFO] Server port: " << port << std::endl;
    
    // Create a client.
    std::shared_ptr<Client> client = std::shared_ptr<Client>(new Client(name));
    // Create a thread to get commands.
    std::future<std::string> command_future = std::async(std::launch::async, get_command);

    print_help();
    std::string command;
    std::future_status status;
    while (true) {
        // Print outputs in the msg queue.
        client->output_message();

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
            std::cerr << "[ERR] Deferred." << std::endl;
            return 1;
        }

        // Parse the command.
        if (command.empty()) {
            continue;
        }
        try {
            if (!process_command(command, client, addr, port)) {
                break;
            }
        } catch (std::exception &e) {
            std::cerr << "[ERR] " << e.what() << std::endl;
        }
    }
}
