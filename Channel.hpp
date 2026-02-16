#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel 
{
    private:
        std::string _name;
        std::set<int> _clients; // fd участников
        std::set<int> _operators;  // fd операторов
        std::string _key;       // для +k
        unsigned int _limit;    // для +l
        bool _inviteOnly;       // +i
        bool _topicRestricted;  // +t
        std::set<int> _invited;   // список приглашённых пользователей(после доавления удалить из списка для невхода повторно)
        std::string _topic;       // тема канала

    
    public:
        Channel(const std::string& name);
    
        const std::string& getName() const;
        const std::set<int>& getClients() const;
        const std::set<int>& getOperators() const;

        void addClient(int fd);
        void removeClient(int fd);
        bool hasClient(int fd) const;
        bool empty() const;

        void addOperator(int fd);
        void removeOperator(int fd);
        bool isOperator(int fd) const;

        bool isInviteOnly() const;
        void setInviteOnly(bool val);

        void addInvited(int fd);
        bool isInvited(int fd) const;
        void removeInvited(int fd);

        bool isTopicRestricted() const;
        void setTopicRestricted(bool val);

        void setTopic(const std::string& topic);
        const std::string& getTopic() const;

        void setKey(const std::string& key);
        const std::string& getKey() const;

        void setLimit(unsigned int n);
        unsigned int getLimit() const;

};

#endif
    