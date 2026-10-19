// Server.hpp
#pragma once

#include <map>
#include <new>
#include <poll.h>
#include <string>
#include <vector>
#include <cctype>
#include <fcntl.h>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sstream> //temporary for testing

#include "Client.hpp"
#include "Channel.hpp"


extern volatile sig_atomic_t g_run;


class Server {

    private:
        int                             _port;
        std::string                     _pwd;
        int                             _listenFd;
        std::vector<pollfd>             _pfds;
        std::map<int, Client*>          _clients;
        std::map<std::string, Channel*> _channels;

        Server(const Server& other);
        Server& operator=(const Server& other);

        void    acceptClient(void);
        bool    readFromClient(size_t i);
        void    removeClient(size_t i);
        void    handleCommand(Client& client, const std::string& line);
        bool    writeToClient(size_t i);
        void    setPollOut(int fd, bool on);
        bool    checkDisconnect(size_t i);
        void    broadcast(Channel& chan, const std::string& msg, int exceptFd);

        Channel*    getChannel(const std::string& name);
        Channel*    createChannel(const std::string& name);
        Client*     getClientByNick(const std::string& nick);

        std::string toLower( const std::string& s );

    public:
        Server(int port, const std::string& pwd);
        ~Server(void);

        void    init(void);
        void    run(void);
        void    reply(Client& client, const std::string& msg);


};

// signals handling
void signalHandler( int sig );