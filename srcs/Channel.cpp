#include "Channel.hpp"


Channel::Channel( const std::string& name ) : _name( name ), _topic( "" ), _key( "" ), _limit( 0 ), _inviteOnly( false ), _topicRestricted( false ) {}

Channel::~Channel(void) {}

// getters
const std::string&  Channel::getName(void)           const { return ( _name ); }
const std::string&  Channel::getTopic(void)          const { return ( _topic ); }
const std::string&  Channel::getKey(void)            const { return ( _key ); }
size_t              Channel::getLimit(void)          const { return ( _limit ); }
bool                Channel::isInviteOnly(void)      const { return (_inviteOnly ); }
bool                Channel::isTopicRestricted(void) const { return ( _topicRestricted ); }

// setters
void    Channel::setTopic(const std::string& topic) { _topic = topic; }
void    Channel::setKey(const std::string& key)     { _key = key; }
void    Channel::setLimit(size_t limit)             { _limit = limit; }
void    Channel::setInviteOnly(bool value)          { _inviteOnly = value; }
void    Channel::setTopicRestricted(bool value)     { _topicRestricted = value; }

// member methods
void                    Channel::addMember(int fd)          { _members.insert(fd); }
void                    Channel::removeMember(int fd)       { _members.erase(fd); _operators.erase(fd); _invited.erase(fd); }
bool                    Channel::isMember(int fd)   const   { return ( _members.find(fd) != _members.end() ); }
size_t                  Channel::memberCount(void)  const   { return ( _members.size() ); }
bool                    Channel::isEmpty(void)      const   { return ( _members.empty() ); }
const std::set<int>&    Channel::getMembers(void)   const   { return ( _members ); }

// operator methods
void    Channel::addOperator(int fd)        { _operators.insert(fd); }
void    Channel::removeOperator(int fd)     { _operators.erase(fd); }
bool    Channel::isOperator(int fd)  const  { return ( _operators.find(fd) != _operators.end() ); }

// invite methods
void    Channel::addInvite(int fd)          { _invited.insert(fd); }
void    Channel::removeInvite(int fd)       { _invited.erase(fd); }
bool    Channel::isInvited(int fd)   const  { return ( _invited.find(fd) != _invited.end() ); }