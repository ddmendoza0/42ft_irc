// Server.cpp
#include "Server.hpp"


volatile sig_atomic_t g_run = 1;

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

    for ( std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it )
        delete it->second;
    _channels.clear();

    for ( std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it  )
        delete it->second;
    _clients.clear();
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
    while ( g_run )
    {
        int ret = poll(&_pfds[0], _pfds.size(), -1);
        
        if ( ret < 0 && !g_run )
            break ;

        if ( ret < 0 )
            throw std::runtime_error("Error: poll failed");

        for ( size_t i = 0; i < _pfds.size(); i++ )
        {
            //server listener
            if ( _pfds[i].fd == _listenFd )
            {
                if ( _pfds[i].revents & POLLIN )
                    acceptClient();
                continue;
            }
            
            //client
            if ( _pfds[i].revents & POLLIN )
            {
                if ( readFromClient(i) )
                {
                    i--;
                    continue ;
                }
            }
            else if ( _pfds[i].revents & POLLOUT )
            {
                if ( writeToClient(i) )
                {
                    i--;
                    continue ;
                }
            }
            else if ( _pfds[i].revents & ( POLLHUP | POLLERR ) )
            {
                removeClient(i);
                i--;
                continue ;
            }

            if ( checkDisconnect(i) ) 
            { 
                i--; 
                continue; 
            }
        }
    }

    std::cout << "Shutting down" << std::endl;
}

void    Server::acceptClient( void )
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int new_fd = accept(_listenFd, reinterpret_cast<struct sockaddr*>(&addr), &len);
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

    std::string hostname = inet_ntoa(addr.sin_addr);

    try
    {
        _clients[new_fd] = new Client(new_fd, hostname);
    }
    catch ( const std::bad_alloc& )
    {
        close(new_fd);
        std::cerr << "Error: memory allocation error" << std::endl;
        return ;
    }

    pollfd p;
    p.fd = new_fd;
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);

    std::cout << "Client connected: " << new_fd << " " << hostname << std::endl;

    return ;
}

bool    Server::readFromClient(size_t i)
{
    char buf[512];

    ssize_t ret = recv(_pfds[i].fd, buf, sizeof(buf), 0);
    if ( ret <= 0 )
    {
        removeClient(i);
        return ( true );
    }
    
    std::map<int, Client*>::iterator it = _clients.find(_pfds[i].fd);
    if ( it == _clients.end() )
        return ( false );
    Client* client = it->second;
    client->appendInput(std::string(buf, ret));

    std::string& in = client->getInBuf();
    size_t pos;
    while ( !client->shouldDisconnect() && (pos = in.find('\n')) != std::string::npos )
    {
        std::string line = in.substr(0, pos);
        in.erase(0, pos + 1);
        if ( !line.empty() && line[line.size() - 1] == '\r' )
            line.erase(line.size() - 1);
        if ( line.empty() )
            continue ;

        handleCommand(*client, line);
    }
    if ( in.size() > 512 )
    {
        std::cerr << "Client fd " << _pfds[i].fd << " exceeded 512 bytes, dropping" << std::endl;
        removeClient(i);
        return ( true );
    }

    return  ( false );
}

void    Server::removeClient(size_t i)
{
    int fd = _pfds[i].fd;

    std::map<std::string, Channel*>::iterator chit = _channels.begin();
    while ( chit != _channels.end() )
    {
        chit->second->removeMember( fd );
        if ( chit->second->isEmpty() )
        {
            delete chit->second;
            _channels.erase( chit++ );
        }
        else
            ++chit;
    }

    std::map<int, Client*>::iterator it = _clients.find(fd);
    if ( it != _clients.end() )
    {
        delete it->second;
        _clients.erase(it);
    }


    std::cout << "Client fd " << _pfds[i].fd << " disconected" << std::endl;

    close(fd);
    _pfds.erase(_pfds.begin() + i);
}

void    Server::handleCommand(Client& client, const std::string& line) //garantizado que recibe una linia completa :D
{
    // test channels
    if ( line.compare( 0, 5, "JOIN " ) == 0 )
    {
        std::string name = line.substr( 5 );
        Channel* chan = getChannel( name );
        if ( chan == NULL )
        {
            chan = createChannel( name );
            if ( chan == NULL )
                return ;
            chan->addOperator( client.getFd() );   // first joiner becomes operator
        }
        chan->addMember( client.getFd() );

        std::ostringstream fd;
        fd << client.getFd();
        broadcast( *chan, "fd " + fd.str() + " joined " + chan->getName(), -1 );
        return ;
    }
    
    if ( line == "QUIT" ) //test quit
    {
        reply( client, "ERROR :Closing link" );
        client.markForDisconnect();
        return ;
    }
    reply( client, "echo: " + line ); //test comm
}

void    Server::setPollOut(int fd, bool on)
{
    for ( size_t i = 0; i < _pfds.size(); i++ )
    {
        if ( _pfds[i].fd != fd ) 
            continue;
        
        if ( on )
            _pfds[i].events |= POLLOUT;
        else
            _pfds[i].events &= ~POLLOUT;
    }

    return ;
}

void    Server::reply( Client& client, const std::string& msg )
{
    client.appendOutput( msg + "\r\n" );
    setPollOut( client.getFd(), true );
}

bool    Server::writeToClient( size_t i )
{
    std::map<int, Client*>::iterator it = _clients.find(_pfds[i].fd);
    if ( it == _clients.end() )
        return ( false );
    Client* client = it->second;
    int     fd = client->getFd();

    std::string& out = client->getOutBuf();
    if ( out.empty() )
    {
        setPollOut(fd, false);
        return ( false );
    }

    ssize_t ret = send(fd, out.c_str(), out.size(), 0);
    if ( ret < 0 )
    {
        removeClient(i);
        return ( true );
    }

    out.erase( 0, ret );
    if ( out.empty() )
        setPollOut(fd, false);
    
    if ( checkDisconnect(i) )
        return ( true );

    return ( false );
}

bool    Server::checkDisconnect(size_t i)
{
    std::map<int, Client*>::iterator it = _clients.find(_pfds[i].fd);
    if ( it == _clients.end() )
        return ( false );

    Client* client = it->second;
    std::string& out = client->getOutBuf();
    if ( client->shouldDisconnect() && out.empty() )
    {
        removeClient(i);
        return ( true );
    }
    return ( false );

}

void    Server::broadcast( Channel& chan, const std::string& msg, int exceptFd )
{
    const std::set<int> &chan_mem = chan.getMembers();
    for ( std::set<int>::const_iterator mit = chan_mem.begin(); mit != chan_mem.end(); ++mit )
    {
        if ( *mit == exceptFd )
            continue ;

        std::map<int, Client*>::iterator cit = _clients.find(*mit);
        if ( cit == _clients.end() )
            continue ;

        Client* client = cit->second;
        reply(*client, msg);
    }
}

std::string Server::toLower( const std::string& s )
{
    std::string result = s;
    for ( size_t i = 0; i < result.size(); i++ )
        result[i] = std::tolower( static_cast<unsigned char>( result[i] ) );
    return ( result );
}

Channel*    Server::getChannel(const std::string& name) 
{
    std::map<std::string, Channel*>::iterator it = _channels.find(toLower(name));
    if ( it == _channels.end() )
        return ( NULL );
    return ( it->second );
}

Channel*    Server::createChannel(const std::string& name) 
{
    Channel* channel = NULL;
    try
    {
        channel = new Channel(name);
        _channels[ toLower(name) ] = channel;
    }
    catch ( const std::bad_alloc& )
    {
        delete channel;
        std::cerr << "Error: memory allocation error" << std::endl;
        return ( NULL );
    }

    return ( channel );
}

Client*     Server::getClientByNick(const std::string& nick) 
{
    std::string key = toLower(nick);

    std::map<int, Client*>::iterator it;
    for ( it = _clients.begin(); it != _clients.end(); ++it )
    {
        if ( toLower(it->second->getNick()) ==  key )
            return ( it->second );
    } 
    return ( NULL );   
}



// signal handle
void signalHandler( int sig )
{
    (void)sig;
    g_run = 0;
}