// Server.cpp
#include "Server.hpp"

Server::Server( int port, const std::string& pwd ) : _port( port ), _pwd( pwd ), _listenFd( -1 ) {}

Server::~Server( void )
{
    if ( _listenFd != -1 )
        close(_listenFd);
    
    for ( size_t i = 0; i < _pfds.size(); i++ )
    {
        if ( _pfds[i].fd != _listenFd )
            close(_pfds[i].fd);
    }
}

void    Server::init( void )
{
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if ( _listenFd == -1 )
        throw std::runtime_error("Error: socket failed");

    int opt = 1;
    if ( setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 )
        throw std::runtime_error("Error: setsockopt failed");

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ( bind(_listenFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0 )
        throw std::runtime_error("Error: bind failed");

    if ( listen(_listenFd, SOMAXCONN) < 0 )
        throw std::runtime_error("Error: listen failed");

    if ( fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0 )
        throw std::runtime_error("Error: fcntl failed");

    pollfd p;
    p.fd = _listenFd;
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);

    return ;
}

void    Server::run( void )
{
    while ( true )
    {
        int ret = poll(&_pfds[0], _pfds.size(), -1);
        
        if ( ret < 0 )
            throw std::runtime_error("Error: poll failed");

        for ( size_t i = 0; i < _pfds.size(); i++ )
        {
            if ( !(_pfds[i].revents & POLLIN) )
                continue ;

            if ( _pfds[i].fd == _listenFd )
                acceptClient();
            else
                std::cout << "WHAT?! " << _pfds[i].fd << std::endl;
        }
    }
}

void    Server::acceptClient( void )
{
    int new_fd = accept(_listenFd, NULL, NULL);
    if ( new_fd < 0 )
    {
        std::cerr << "Error: could not accept Client" << std::endl;
        return ;
    }

    if ( fcntl(new_fd, F_SETFL, O_NONBLOCK) < 0 )
    {
        close( new_fd );
        std::cerr << "Error: fcntl failed" << std::endl;
        return ;
    }

    pollfd p;
    p.fd = new_fd;
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);

    std::cout << "Someones knocking " << new_fd << std::endl;

    return ;
}