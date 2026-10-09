#pragma once

class Client {
    public:
        int         fd;
        std::string inbuf, outbuf;
        std::string nick, user, realname, hostname;
        bool        passOk, registered;
        // helpers: queue(msg) appends msg + "\r\n" to outbuf; prefix() returns nick!user@host
};