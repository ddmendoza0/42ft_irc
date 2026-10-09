
class Server; 

class Channel {
    public:
        std::string       name, topic, key;
        size_t            limit;           // 0 = no limit
        bool              inviteOnly, topicOpOnly;
        std::set<int>     members, operators, invited;   // store fds
        // helpers: broadcast(server, msg, exceptFd), isMember, isOperator, removeMember
};