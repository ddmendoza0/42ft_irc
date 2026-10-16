// main.cpp
#include <string>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <iostream>

#include "Server.hpp"

int main( int argc, char *argv[] )
{
    if ( argc != 3 )
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return ( 1 );
    }

    if ( !std::isdigit(static_cast<unsigned char>(argv[1][0])) )
    {
        std::cerr << "Error: invalid port" << std::endl;
        return ( 1 );
    }

    char*   endptr;
    long int    port = std::strtol(argv[1], &endptr, 10);
    if ( *endptr != '\0' || ( port < 1 || port > 65535 ))
    {
        std::cerr << "Error: invalid port" << std::endl;
        return ( 1 );
    }

    if ( !argv[2][0] )
    {
        std::cerr << "Error: password cannot be empty" << std::endl;
        return ( 1 );
    }

    Server server(static_cast<int>(port), argv[2]);
    try
    {
        server.init();

        signal(SIGPIPE, SIG_IGN);

        server.run();
    }
    catch ( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return ( 1 );
    }

    return ( 0 );
}