#ifndef __DEF_HPP__
#define __DEF_HPP__

#define MAX_BUFFER_SIZE 4096
#define MAX_CLIENT_NUM 255
#define MAX_EPOLL_EVENTS 1
#define TIMEOUT 200

#define SERVER_ID 0
#define SERVER_ADDR INADDR_ANY
#define SERVER_PORT 2024

#define DIVISION_SIGNAL '\0'

#define data_t std::vector<std::string>
#define send_res_t std::pair<uint16_t, ssize_t>

#define cast_sockaddr_in(addr) reinterpret_cast<sockaddr *>(&(addr))

#endif
