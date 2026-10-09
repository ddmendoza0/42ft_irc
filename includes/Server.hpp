// Server.hpp
#pragma once

#include <map>
#include <new>
#include <poll.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "Client.hpp"

class Server {

    private:
        int                     _port;
        std::string             _pwd;
        int                     _listenFd;
        std::vector<pollfd>     _pfds;
        std::map<int, Client*>  _clients;

        Server(const Server& other);
        Server& operator=(const Server& other);

        void    acceptClient(void);
        bool    readFromClient(size_t i);
        void    removeClient(size_t i);
        void    handleCommand(Client& client, const std::string& line);
        void    reply(Client& client, const std::string& msg);

    public:
        Server(int port, const std::string& pwd);
        ~Server(void);

        void    init(void);
        void    run(void);

};