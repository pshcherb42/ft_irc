#include "Channel.hpp"

Channel::Channel(const std::string& name) : _name(name) {}

const std::string& Channel::getName() const {
    return _name;
}

void Channel::addClient(int fd) {
    _clients.insert(fd);
}

void Channel::removeClient(int fd) {
    _clients.erase(fd);
}

bool Channel::hasClient(int fd) const {
    return _clients.find(fd) != _clients.end();
}

const std::set<int>& Channel::getClients() const {
    return _clients;
}
