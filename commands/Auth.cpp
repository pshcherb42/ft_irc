#include "../Server.hpp"
#include "../Client.hpp"

void Server::cmdPass(int fd, const std::vector<std::string>& params) 
{
    Client* client = _clients[fd];
    
    // Check if enough parameters
    if (params.size() < 2) 
    {
        sendToClient(fd, ":server 461 * PASS :Not enough parameters");
        return;
    }
    
    // Check password
    if (params[1] == _password) 
    {
        client->setAuthenticated(true);
        std::cout << "Client fd " << fd << " authenticated" << std::endl;
    } 
    else 
    {
        sendToClient(fd, ":server 464 * :Password incorrect");
        removeClient(fd);  // Wrong password = disconnect
    }
}

void Server::cmdNick(int fd, const std::vector<std::string>& params)
{
    std::map<int, Client*>::iterator itClient = _clients.find(fd);
    if (itClient == _clients.end() || itClient->second == NULL)
        return;

    Client* client = itClient->second;

    // Проверка параметров
    if (params.size() < 2 || params[1].empty())
    {
        sendToClient(fd, ":server 431 * :No nickname given");
        return;
    }

    std::string newNick = params[1];

    // Проверка на занятость ника
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->first != fd && it->second && it->second->getNickname() == newNick)
        {
            sendToClient(fd, ":server 433 * " + newNick + " :Nickname is already in use");
            return;
        }
    }

    std::string oldNick = client->getNickname();
    bool hadOldNick = !oldNick.empty();

    // Если ник не изменился, ничего не делаем
    if (hadOldNick && oldNick == newNick)
        return;

    // Сохраняем старый prefix ДО смены ника
    std::string oldPrefix;
    if (hadOldNick)
        oldPrefix = ":" + oldNick + "!" + client->getUsername() + "@localhost";

    // Меняем ник
    client->setNickname(newNick);
    std::cout << "Client fd " << fd << " set nickname: " << newNick << std::endl;

    // Если это первая полная регистрация
    if (!client->getUsername().empty() && !client->isRegistered())
    {
        client->setRegistered(true);
        sendToClient(fd, ":server 001 " + newNick + " :Welcome to the IRC Network");
        std::cout << "Client fd " << fd << " is now fully registered" << std::endl;
        return;
    }

    // Если ник меняется уже после того, как он был установлен,
    // нужно разослать NICK всем, кто должен это видеть
    if (hadOldNick)
    {
        std::string nickMsg = oldPrefix + " NICK :" + newNick;

        // Чтобы не слать одному и тому же fd несколько раз,
        // соберём получателей во множество
        std::set<int> recipients;
        recipients.insert(fd); // сам клиент тоже должен получить NICK

        for (std::map<std::string, Channel>::iterator chIt = _channels.begin(); chIt != _channels.end(); ++chIt)
        {
            Channel& channel = chIt->second;
            if (channel.hasClient(fd))
            {
                const std::set<int>& members = channel.getClients();
                for (std::set<int>::const_iterator m = members.begin(); m != members.end(); ++m)
                    recipients.insert(*m);
            }
        }

        for (std::set<int>::const_iterator r = recipients.begin(); r != recipients.end(); ++r)
            sendToClient(*r, nickMsg);
    }
}

void Server::cmdUser(int fd, const std::vector<std::string>& params) 
{
    Client* client = _clients[fd];  // FIX: You forgot to initialize!
    
    // 1. Check if authenticated
    if (!client->isAuthenticated()) 
    {
        sendToClient(fd, ":server 451 * :You have not registered");
        return;
    }
    
    // 2. Check if enough parameters (USER username hostname servername :realname)
    if (params.size() < 5) 
    {
        sendToClient(fd, ":server 461 * USER :Not enough parameters");
        return;
    }
    
    // 3. Set username (params[1])
    std::string username = params[1];
    client->setUsername(username);
    std::cout << "Client fd " << fd << " set username: " << username << std::endl;
    
    // 4. Set realname (params[4] onwards, removing leading ':')
    std::string realname = params[4];
    if (realname[0] == ':')
        realname = realname.substr(1);  // Remove the ':'
    
    // If there are more parameters, join them (realname can have spaces)
    for (size_t i = 5; i < params.size(); i++)
        realname += " " + params[i];
    
    client->setRealname(realname);
    std::cout << "Client fd " << fd << " set realname: " << realname << std::endl;
    
    // 5. If both nickname AND username are set, mark as registered
    if (!client->getNickname().empty() && !client->isRegistered()) 
    {
        client->setRegistered(true);
        std::string nick = client->getNickname();
        sendToClient(fd, ":server 001 " + nick + " :Welcome to the IRC Network");
        sendToClient(fd, ":server 002 " + nick + " :Your host is server");
        sendToClient(fd, ":server 003 " + nick + " :This server was created today");
        sendToClient(fd, ":server 004 " + nick + " server 1.0 o o");
        std::cout << "Client fd " << fd << " is now fully registered" << std::endl;
    }
}