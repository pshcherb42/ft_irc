#include "../../Server.hpp"
#include "../../Client.hpp"

void Server::cmdPart(int fd, const std::vector<std::string>& params) 
{
    if (params.size() < 2)
    {
        sendToClient(fd, ":server 461 PART :Not enough parameters");
        return;
    }
    std::string channelName = params[1];

    // канал существует?
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) 
    {
        sendToClient(fd, ":server 403 " + channelName + " :No such channel");
        return;
    }

    Channel& channel = it->second;

    // клиент в канале?
    if (!channel.hasClient(fd)) 
    {
        sendToClient(fd, ":server 442 " + channelName + " :You're not on that channel");
        return;
    }

    Client& client = *(_clients[fd]);

    // формируем сообщение
    std::string partMsg = client.getPrefix() + " PART " + channelName;

    // рассылаем всем участникам канала
    const std::set<int>& members = channel.getClients();
    for (std::set<int>::const_iterator it2 = members.begin(); it2 != members.end(); ++it2)
        sendToClient(*it2, partMsg);

    // удаляем клиента
    channel.removeClient(fd);

    std::cout << "Client fd " << fd << " left channel " << channelName << std::endl;

    // если канал пустой — удаляем его
    if (channel.empty())
        _channels.erase(channelName);
}