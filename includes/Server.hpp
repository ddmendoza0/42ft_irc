// Server.hpp
#pragma once

#include <poll.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>

class Server {

    private:
        int                 _port;
        std::string         _pwd;
        int                 _listenFd;
        std::vector<pollfd> _pfds;

        Server(const Server& other);
        Server& operator=(const Server& other);

        void    acceptClient(void);
        bool    readFromClient(size_t i);
        void    removeClient(size_t i);

    public:
        Server(int port, const std::string& pwd);
        ~Server(void);

        void    init(void);
        void    run(void);

};