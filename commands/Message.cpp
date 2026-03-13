#include "../Server.hpp"
#include "../Client.hpp"

void Server::cmdPrivmsg(int fd, const std::vector<std::string>& params)
{
    // 1. Проверяем, что клиент существует
    std::map<int, Client*>::iterator clientIt = _clients.find(fd);
    if (clientIt == _clients.end() || clientIt->second == NULL)
        return;

    Client* sender = clientIt->second;

    // 2. Проверяем регистрацию
    if (!sender->isRegistered())
    {
        sendToClient(fd, ":server 451 * :You have not registered");
        return;
    }

    // 3. Проверяем параметры
    if (params.size() < 3)
    {
        sendToClient(fd, ":server 461 * PRIVMSG :Not enough parameters");
        return;
    }

    std::string target = params[1];
    std::string message = params[2];

    for (size_t i = 3; i < params.size(); i++)
        message += " " + params[i];

    if (!message.empty() && message[0] == ':')
        message.erase(0, 1);

    // Пустое сообщение не отправляем
    if (message.empty())
    {
        sendToClient(fd, ":server 412 :No text to send");
        return;
    }

    // 4. Сообщение в канал
    if (!target.empty() && target[0] == '#')
    {
        std::map<std::string, Channel>::iterator chIt = _channels.find(target);
        if (chIt == _channels.end())
        {
            sendToClient(fd, ":server 403 " + target + " :No such channel");
            return;
        }

        Channel& channel = chIt->second;

        if (!channel.hasClient(fd))
        {
            sendToClient(fd, ":server 442 " + target + " :You're not on that channel");
            return;
        }

        std::string fullMsg = sender->getPrefix() + " PRIVMSG " + target + " :" + message;

        const std::set<int>& members = channel.getClients();
        for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            if (*it != fd) // не шлём обратно отправителю
                sendToClient(*it, fullMsg);
        }
    }
    else
    {
        // 5. Личное сообщение пользователю
        bool found = false;

        std::cout << "PRIVMSG target = [" << target << "]" << std::endl;

        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            if (it->second && it->second->getNickname() == target)
            {
                std::string fullMsg = sender->getPrefix() + " PRIVMSG " + target + " :" + message;
                sendToClient(it->first, fullMsg);
                found = true;
                break;
            }
        }

        if (!found)
            sendToClient(fd, ":server 401 " + target + " :No such nick");
    }
}

void Server::cmdNotice(int fd, const std::vector<std::string>& params)
{
    std::map<int, Client*>::iterator clientIt = _clients.find(fd);
    if (clientIt == _clients.end() || clientIt->second == NULL)
        return;

    Client* sender = clientIt->second;

    // NOTICE не отправляет ошибок, просто игнорируем
    if (!sender->isRegistered())
        return;

    if (params.size() < 3)
        return;

    std::string target = params[1];
    std::string message = params[2];

    for (size_t i = 3; i < params.size(); ++i)
        message += " " + params[i];

    if (!message.empty() && message[0] == ':')
        message.erase(0, 1);

    if (message.empty())
        return;

    // NOTICE в канал
    if (!target.empty() && target[0] == '#')
    {
        std::map<std::string, Channel>::iterator chIt = _channels.find(target);
        if (chIt == _channels.end())
            return;

        Channel& channel = chIt->second;

        if (!channel.hasClient(fd))
            return;

        std::string fullMsg = sender->getPrefix() + " NOTICE " + target + " :" + message;

        const std::set<int>& members = channel.getClients();
        for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            if (*it != fd)
                sendToClient(*it, fullMsg);
        }
    }
    else
    {
        // NOTICE пользователю
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            if (it->second && it->second->getNickname() == target)
            {
                std::string fullMsg = sender->getPrefix() + " NOTICE " + target + " :" + message;
                sendToClient(it->first, fullMsg);
                return;
            }
        }
    }
}
