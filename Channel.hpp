#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
    private:
        std::string _name;
        std::set<int> _clients; // fd участников
    
    public:
        Channel(const std::string& name);
    
        const std::string& getName() const;

        void addClient(int fd);
        void removeClient(int fd);
        bool hasClient(int fd) const;

        const std::set<int>& getClients() const;
};

#endif
    