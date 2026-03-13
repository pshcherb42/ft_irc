#include "../../Server.hpp"
#include "../../Client.hpp"

void Server::cmdKick(int fd, const std::vector<std::string>& params)
{
    if (params.size() < 3)
    {
        sendToClient(fd, ":server 461 KICK :Not enough parameters");
        return;
    }

    std::map<int, Client*>::iterator senderIt = _clients.find(fd);
    if (senderIt == _clients.end() || senderIt->second == NULL)
        return;

    Client& sender = *(senderIt->second);

    std::string channelName = params[1];
    std::string targetNick = params[2];

    std::map<std::string, Channel>::iterator chIt = _channels.find(channelName);
    if (chIt == _channels.end())
    {
        sendToClient(fd, ":server 403 " + channelName + " :No such channel");
        return;
    }

    Channel& channel = chIt->second;

    if (!channel.hasClient(fd))
    {
        sendToClient(fd, ":server 442 " + channelName + " :You're not on that channel");
        return;
    }

    if (!channel.isOperator(fd))
    {
        sendToClient(fd, ":server 482 " + channelName + " :You're not channel operator");
        return;
    }

    int targetFd = -1;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second && it->second->getNickname() == targetNick)
        {
            targetFd = it->first;
            break;
        }
    }

    if (targetFd == -1 || !channel.hasClient(targetFd))
    {
        sendToClient(fd, ":server 441 " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    std::string reason = "Kicked";
    if (params.size() > 3)
    {
        reason = params[3];
        for (size_t i = 4; i < params.size(); ++i)
            reason += " " + params[i];

        if (!reason.empty() && reason[0] == ':')
            reason.erase(0, 1);
    }

    std::string kickMsg = sender.getPrefix() + " KICK " + channelName + " " +
                          targetNick + " :" + reason;

    const std::set<int>& members = channel.getClients();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
        sendToClient(*it, kickMsg);

    channel.removeClient(targetFd);
}