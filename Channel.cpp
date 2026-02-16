#include "Channel.hpp"

Channel::Channel(const std::string& name)
    : _name(name),
      _limit(0),
      _inviteOnly(false),
      _topicRestricted(false)
{}

const std::string& Channel::getName() const { return (_name); }

const std::set<int>& Channel::getClients() const { return (_clients); }

const std::set<int>& Channel::getOperators() const { return (_operators); }


void Channel::addClient(int fd) 
{ 
    _clients.insert(fd);

    // если это первый клиент в канале — делаем его оператором
    if (_clients.size() == 1)
        _operators.insert(fd);
}

void Channel::removeClient(int fd) 
{ 
    _clients.erase(fd);
    _operators.erase(fd); // если был оператор, убираем
}

bool Channel::hasClient(int fd) const { return (_clients.find(fd) != _clients.end()); }

bool Channel::empty() const { return (_clients.empty()); }


void Channel::addOperator(int fd) 
{
    if (hasClient(fd))
        _operators.insert(fd);
}

void Channel::removeOperator(int fd) { _operators.erase(fd); }

bool Channel::isOperator(int fd) const { return (_operators.find(fd) != _operators.end()); }

bool Channel::isInviteOnly() const { return (_inviteOnly); }
void Channel::setInviteOnly(bool val) { _inviteOnly = val; }

void Channel::addInvited(int fd) { _invited.insert(fd); }
bool Channel::isInvited(int fd) const { return _invited.find(fd) != _invited.end(); }
void Channel::removeInvited(int fd) { _invited.erase(fd); }


bool Channel::isTopicRestricted() const { return (_topicRestricted); }
void Channel::setTopicRestricted(bool val) { _topicRestricted = val; }

void Channel::setTopic(const std::string& topic) { _topic = topic; }// Устанавливает новую тему канала
const std::string& Channel::getTopic() const { return (_topic); }// Возвращает текущую тему канала


void Channel::setKey(const std::string& key) { _key = key; }
const std::string& Channel::getKey() const { return (_key); }

void Channel::setLimit(unsigned int n) { _limit = n; }
unsigned int Channel::getLimit() const { return (_limit); }

