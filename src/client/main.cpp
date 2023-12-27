#include "Client.hpp"
#include <iostream>

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
    std::cout << "1. connect: Connect to the server." << std::endl
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
    Client client(name);

    print_help();
    std::string input;
    while (true) {
        std::cout << "client> ";
        std::getline(std::cin, input);
        if (input == "") {
            continue;
        }
        std::string choice = input.substr(0, input.find(' '));
        try {
            switch (get_choice(choice)) {
                case Choice::CONNECT : {
                    // Connect to the server.
                    client.connect_to_server(addr, port);
                    break;
                }
                case Choice::DISCONNECT : {
                    // Disconnect from the server.
                    client.disconnect_from_server();
                    break;
                }
                case Choice::GET_TIME : {
                    // Get the time from the server.
                    client.get_time();
                    break;
                }
                case Choice::GET_NAME : {
                    // Get the name of the server.
                    client.get_name();
                    break;
                }
                case Choice::GET_CLIENT_LIST : {
                    // Get the list of the clients.
                    client.get_client_list();
                    break;
                }
                case Choice::SEND_MESSAGE : {
                    int pos1 = input.find(' ');
                    int pos2 = input.find('\"', pos1 + 1);
                    int pos3 = input.find('\"', pos2 + 1);
                    if (pos1 == std::string::npos || pos2 == std::string::npos || pos3 == std::string::npos) {
                        std::cerr << "[WARN] Invalid input: " << input << std::endl;
                        break;
                    }
                    // Get the id and the content.
                    std::string id_str = input.substr(pos1 + 1, pos2 - pos1 - 2);
                    std::string content = input.substr(pos2 + 1, pos3 - pos2 - 1);
                    int id = atoi(id_str.c_str());
                    if (id < 0 || id > 255) {
                        std::cerr << "[WARN] Invalid id." << std::endl;
                        break;
                    }
                    // transform the id to uint8_t.
                    uint8_t receiver_id = (uint8_t)id;
                    std::cout << "[INFO] Sending message \"" << content << "\" to client " << (int)receiver_id << std::endl;
                    client.send_message(receiver_id, content);
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
        } catch (std::exception &e) {
            std::cerr << "[ERR] " << e.what() << std::endl;
        }
    }
}
