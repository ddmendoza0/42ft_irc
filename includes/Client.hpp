// Client.hpp
#pragma once

#include <string>

class Client {
    private:
        int         _fd;
        std::string _inBuf;
        std::string _outBuf;
        std::string _nick;
        std::string _user;
        std::string _realname;
        std::string _hostname;
        bool        _passOk;
        bool        _registered;

        Client(const Client& other);
        Client& operator=(const Client& other);
    
    public:
        Client(int fd, const std::string& hostname);
        ~Client(void);

        // getters
        int                 getFd( void ) const;
        const std::string&  getNick( void ) const;
        const std::string&  getUser( void ) const;
        const std::string&  getRealname( void ) const;
        const std::string&  getHostname( void ) const;
        bool                isPassOk( void ) const;
        bool                isRegistered( void ) const;

        // setters
        void    setNick( const std::string& nick );
        void    setUser( const std::string& user );
        void    setRealname( const std::string& realname );
        void    setPassOk( bool value );
        void    setRegistered( bool value );

        // buffers
        std::string&    getInBuf( void );
        std::string&    getOutBuf( void );
        void            appendInput( const std::string& data );
        void            appendOutput( const std::string& data );
    };