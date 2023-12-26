#ifndef __DEF_HPP__
#define __DEF_HPP__

#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENT_NUM 255

#define SERVER_ID 0
#define SERVER_ADDR INADDR_ANY
#define SERVER_PORT 2024

#define data_t std::vector<std::string>
#define send_res_t std::pair<unsigned char, size_t>
#define client_map_t std::map<unsigned char, std::unique_ptr<ClientInfo> >

#define cast_sockaddr_in(addr) reinterpret_cast<sockaddr *>(&(addr))

#endif
