#include "../../Server.hpp"
#include "../../Client.hpp"

bool Server::isValidChannelName(const std::string& name)
{
    if (name.empty())
        return false;

    // && name[0] != '&' tampoco comprobamos eso porque es un caso de canal especial y el ejercicio nos pide funcionamiento basico
    if (name[0] != '#')
        return false;

    if (name.length() > 200)
        return false;

    // by the protocol this is the correct version but it means we would have to supoort multiple /join and Its not stated in the subject
    // for (size_t i = 0; i < name.length(); i++)
    // {
    //     if (name[i] == ' ' || name[i] == ',' || name[i] == '\a')
    //         return false;
    // }
    return true;
}

void Server::cmdJoin(int fd, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendToClient(fd, ":server 461 * JOIN :Not enough parameters");
        return;
    }

    if (_clients.find(fd) == _clients.end())
        return;

    Client* client = _clients[fd];

    if (!client->isRegistered())
    {
        sendToClient(fd, ":server 451 * :You have not registered");
        return;
    }

    std::string channelName = params[1];

    if (!isValidChannelName(channelName))
    {
        sendToClient(fd, ":server 403 " + client->getNickname() + " " + channelName + " :Invalid channel name");
        return;
    }

    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    bool channelExists = (it != _channels.end());

    // Check key BEFORE creating anything
    if (channelExists && !it->second.getKey().empty())
    {
        std::string givenKey = (params.size() >= 3) ? params[2] : "";
        if (givenKey != it->second.getKey())
        {
            sendToClient(fd, ":server 475 " + client->getNickname() + " " + channelName + " :Cannot join channel (+k) key requested");
            return;
        }
    }

    // Check limit BEFORE creating anything
    if (channelExists && it->second.getLimit() > 0 && it->second.getClients().size() >= it->second.getLimit())
    {
        sendToClient(fd, ":server 471 " + client->getNickname() + " " + channelName + " :Cannot join channel (+l) reached the limit");
        return;
    }

    // Check invite-only BEFORE creating anything
    if (channelExists && it->second.isInviteOnly() && !it->second.isInvited(fd))
    {
        sendToClient(fd, ":server 473 " + client->getNickname() + " " + channelName + " :Cannot join channel (+i) only invited people");
        return;
    }

    // Only NOW create if it doesn't exist
    if (!channelExists)
    {
        _channels.insert(std::make_pair(channelName, Channel(channelName)));
        it = _channels.find(channelName);
    }

    Channel& channel = it->second;

    // Client already in channel — nothing to do
    if (channel.hasClient(fd))
        return;

    channel.addClient(fd);
    channel.removeInvited(fd);

    // If first member, make them operator
    if (channel.getClients().size() == 1)
        channel.addOperator(fd);

    std::string joinMsg = client->getPrefix() + " JOIN " + channelName;

    // Broadcast JOIN to all members (including the new one)
    const std::set<int>& members = channel.getClients();
    for (std::set<int>::const_iterator m = members.begin(); m != members.end(); ++m)
        sendToClient(*m, joinMsg);

    // Send topic to new client if set
    if (!channel.getTopic().empty())
        sendToClient(fd, ":server 332 " + client->getNickname() + " " + channelName + " :" + channel.getTopic());
    else
        sendToClient(fd, ":server 331 " + client->getNickname() + " " + channelName + " :No topic is set");

    // Send NAMES list (353) to new client
    std::string namesList = "";
    for (std::set<int>::const_iterator m = members.begin(); m != members.end(); ++m)
    {
        if (_clients.find(*m) != _clients.end())
        {
            if (!namesList.empty()) namesList += " ";
            if (channel.isOperator(*m)) namesList += "@";
            namesList += _clients[*m]->getNickname();
        }
    }
    sendToClient(fd, ":server 353 " + client->getNickname() + " = " + channelName + " :" + namesList);
    sendToClient(fd, ":server 366 " + client->getNickname() + " " + channelName + " :End of /NAMES list");

    std::cout << "Client fd " << fd << " joined channel " << channelName << std::endl;
}