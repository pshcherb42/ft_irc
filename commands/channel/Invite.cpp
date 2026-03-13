#include "../../Server.hpp"
#include "../../Client.hpp"

void Server::cmdInvite(int fd, const std::vector<std::string>& params)
{
    if (params.size() < 3)
    {
        sendToClient(fd, ":server 461 INVITE :Not enough parameters");
        return;
    }

    std::string nick = params[1];
    std::string channelName = params[2];

    if (_channels.find(channelName) == _channels.end())
    {
        sendToClient(fd, ":server 403 " + channelName + " :No such channel");
        return;
    }

    std::map<std::string, Channel>::iterator it = _channels.find(channelName);

    if (it == _channels.end())
    {
        sendToClient(fd, ":server 403 " + channelName + " :No such channel");
        return;
    }

    Channel& channel = it->second;


    // Проверка: отправитель в канале
    if (!channel.hasClient(fd))
    {
        sendToClient(fd, ":server 442 " + channelName + " :You're not on that channel");
        return;
    }

    // Проверка: отправитель оператор
    if (!channel.isOperator(fd))
    {
        sendToClient(fd, ":server 482 " + channelName + " :You're not channel operator");
        return;
    }

    // 🔎 Ищем пользователя по нику вручную
    int targetFd = -1;
    for (std::map<int, Client*>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nick)
        {
            targetFd = it->first;
            break;
        }
    }

    if (targetFd == -1)
    {
        sendToClient(fd, ":server 401 " + nick + " :No such nick");
        return;
    }

    if (channel.hasClient(targetFd))
    {
        sendToClient(fd, ":server 443 " + nick + " " + channelName + " :is already on channel");
        return;
    }

    channel.addInvited(targetFd);

    sendToClient(targetFd,
        ":" + _clients[fd]->getNickname() +
        " INVITE " + nick + " " + channelName);

    sendToClient(fd,
        ":server 341 " + nick + " " + channelName);
}