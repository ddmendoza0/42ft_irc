// Client.cpp
#include "Client.hpp"

Client::Client( int fd, const std::string& hostname ) : _fd( fd ), _inBuf( "" ), _outBuf( "" ), _nick( "" ), _user( "" ),  _realname( "" ), _hostname( hostname ), _passOk( false ), _registered( false ) {}

Client::~Client( void ) {}

// getters
int                 Client::getFd( void ) const        { return ( _fd ); }
const std::string&  Client::getNick( void ) const      { return ( _nick ); }
const std::string&  Client::getUser( void ) const      { return ( _user ); }
const std::string&  Client::getRealname( void ) const  { return ( _realname ); }
const std::string&  Client::getHostname( void ) const  { return ( _hostname ); }
bool                Client::isPassOk( void ) const     { return ( _passOk ); }
bool                Client::isRegistered( void ) const { return ( _registered ); }

// setters
void    Client::setNick( const std::string& nick )          { _nick = nick; }
void    Client::setUser( const std::string& user )          { _user = user; }
void    Client::setRealname( const std::string& realname )  { _realname = realname; }
void    Client::setPassOk( bool value )                     { _passOk = value; }
void    Client::setRegistered( bool value )                 { _registered = value; }

// buffers
std::string&    Client::getInBuf( void )                          { return ( _inBuf ); }
std::string&    Client::getOutBuf( void )                         { return ( _outBuf ); }
void            Client::appendInput( const std::string& data )    { _inBuf += data; }
void            Client::appendOutput( const std::string& data )   { _outBuf += data; }