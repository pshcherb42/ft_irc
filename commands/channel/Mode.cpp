#include "../../Server.hpp"
#include "../../Client.hpp"
#include <sstream>

void Server::cmdMode(int fd, const std::vector<std::string>& params)
{
    // MODE без target вообще нельзя
    if (params.size() < 2)
    {
        sendToClient(fd, ":server 461 MODE :Not enough parameters");
        return;
    }

    std::string channelName = params[1];

    // В вашем проекте пока работаем только с channel modes
    if (channelName.empty() || channelName[0] != '#')
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

    std::map<int, Client*>::iterator clientIt = _clients.find(fd);
    if (clientIt == _clients.end() || clientIt->second == NULL)
        return;

    Client& sender = *(clientIt->second);

    // -------------------------------------------------
    // 1. Если пришло только MODE #channel
    //    => это запрос текущих mode канала
    // -------------------------------------------------
    if (params.size() == 2)
    {
        std::string currentModes = "+";

        if (channel.isInviteOnly())
            currentModes += "i";
        if (channel.isTopicRestricted())
            currentModes += "t";
        if (!channel.getKey().empty())
            currentModes += "k";
        if (channel.getLimit() > 0)
            currentModes += "l";

        std::string modeReply = ":server 324 " + sender.getNickname() + " " +
                                channelName + " " + currentModes;

        if (!channel.getKey().empty())
            modeReply += " " + channel.getKey();
        if (channel.getLimit() > 0)
        {
            std::stringstream ss;
            ss << channel.getLimit();
            modeReply += " " + ss.str();
        }

        sendToClient(fd, modeReply);
        return;
    }

    // -------------------------------------------------
    // 2. Начиная с этого места MODE меняет режимы
    // -------------------------------------------------
    if (!channel.isOperator(fd))
    {
        sendToClient(fd, ":server 482 " + channelName + " :You're not a channel operator");
        return;
    }

    std::string modeChanges = params[2];
    bool adding = true;
    size_t paramIndex = 3;

    std::string appliedModes = "";
    std::vector<std::string> appliedParams;

    for (size_t i = 0; i < modeChanges.size(); ++i)
    {
        char c = modeChanges[i];

        if (c == '+')
        {
            adding = true;
            appliedModes += c;
        }
        else if (c == '-')
        {
            adding = false;
            appliedModes += c;
        }
        else
        {
            switch (c)
            {
                case 'i':
                    channel.setInviteOnly(adding);
                    appliedModes += 'i';
                    break;

                case 't':
                    channel.setTopicRestricted(adding);
                    appliedModes += 't';
                    break;

                case 'k':
                    if (adding)
                    {
                        if (paramIndex >= params.size())
                        {
                            sendToClient(fd, ":server 461 MODE :Not enough parameters");
                            return;
                        }
                        channel.setKey(params[paramIndex]);
                        appliedModes += 'k';
                        appliedParams.push_back(params[paramIndex]);
                        paramIndex++;
                    }
                    else
                    {
                        channel.setKey("");
                        appliedModes += 'k';
                    }
                    break;

                case 'l':
                    if (adding)
                    {
                        if (paramIndex >= params.size())
                        {
                            sendToClient(fd, ":server 461 MODE :Not enough parameters");
                            return;
                        }

                        std::stringstream ss(params[paramIndex]);
                        unsigned int limit = 0;
                        ss >> limit;

                        channel.setLimit(limit);
                        appliedModes += 'l';
                        appliedParams.push_back(params[paramIndex]);
                        paramIndex++;
                    }
                    else
                    {
                        channel.setLimit(0);
                        appliedModes += 'l';
                    }
                    break;

                case 'o':
                    if (paramIndex >= params.size())
                    {
                        sendToClient(fd, ":server 461 MODE :Not enough parameters");
                        return;
                    }

                    {
                        std::string nick = params[paramIndex];
                        int targetFd = -1;

                        for (std::map<int, Client*>::iterator itClient = _clients.begin();
                             itClient != _clients.end(); ++itClient)
                        {
                            if (itClient->second && itClient->second->getNickname() == nick)
                            {
                                targetFd = itClient->first;
                                break;
                            }
                        }

                        if (targetFd == -1 || !channel.hasClient(targetFd))
                        {
                            sendToClient(fd, ":server 441 " + nick + " " + channelName + " :They aren't on that channel");
                            return;
                        }

                        if (adding)
                            channel.addOperator(targetFd);
                        else
                            channel.removeOperator(targetFd);

                        appliedModes += 'o';
                        appliedParams.push_back(nick);
                        paramIndex++;
                    }
                    break;

                default:
                    sendToClient(fd, ":server 472 " + std::string(1, c) + " :is unknown mode char to me");
                    return;
            }
        }
    }

    if (appliedModes.empty() || appliedModes == "+" || appliedModes == "-")
        return;

    std::string modeMsg = sender.getPrefix() + " MODE " + channelName + " " + appliedModes;

    for (size_t i = 0; i < appliedParams.size(); ++i)
        modeMsg += " " + appliedParams[i];

    modeMsg += "\r\n";

    const std::set<int>& members = channel.getClients();
    for (std::set<int>::const_iterator it2 = members.begin(); it2 != members.end(); ++it2)
        sendToClient(*it2, modeMsg);
}