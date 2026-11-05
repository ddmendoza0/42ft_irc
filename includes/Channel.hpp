#pragma once


#include <string>
#include <set>

class Channel {

    private:

	std::string     _name;
	std::string     _key;
	std::string     _topic;

        size_t          _limit;           // 0 = no limit

        bool            _inviteOnly;
	bool		_hasKey;
	bool		_hasTopic;
        bool            _topicRestricted;

        std::set<int>   _members;
        std::set<int>   _operators;
        std::set<int>   _invited;

        Channel(const Channel& other);
        Channel& operator=(const Channel& other);

    public:

        Channel(const std::string& name);
        ~Channel(void);

        // getters
        const std::string&	getName(void)		const;
        const std::string&	getKey(void)		const;
        const std::string&	getTopic(void)		const;
        size_t			getLimit(void)		const;
	bool			hasKey(void)		const;
	bool			hasTopic(void)		const;
        bool			isInviteOnly(void)	const;
        bool			isTopicRestricted(void)	const;

        // setters
        void    setKey(const std::string& key);
        void    setTopic(const std::string& topic);
        void    setLimit(size_t limit);
        void    setInviteOnly(bool value);
        void    setTopicRestricted(bool value);

        // member methods
        void                    addMember(int fd);
        void                    removeMember(int fd);
        bool                    isMember(int fd)    const;
        bool                    isEmpty(void)       const;
        size_t                  memberCount(void)   const;
        const std::set<int>&    getMembers(void)    const;

        // operator methods
        void    addOperator(int fd);
        void    removeOperator(int fd);
        bool    isOperator(int fd)  const;

        // invite methods
        void    addInvite(int fd);
        void    removeInvite(int fd);
        bool    isInvited(int fd)   const;

};
