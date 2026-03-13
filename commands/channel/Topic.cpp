#include "../../Server.hpp"
#include "../../Client.hpp"

void Server::cmdTopic(int fd, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendToClient(fd, ":server 461 * TOPIC :Not enough parameters");
        return;
    }

    std::string channelName = params[1];

    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        sendToClient(fd, ":server 403 " + channelName + " :No such channel");
        return;
    }

    Channel& channel = it->second;

    if (!channel.hasClient(fd)) {
        sendToClient(fd, ":server 442 " + channelName + " :You're not on that channel");
        return;
    }

    Client* client = _clients[fd];

    // Если пришёл только канал — показать тему
    if (params.size() == 2) {
        if (channel.getTopic().empty()) {
            sendToClient(fd, ":server 331 " + channelName + " :No topic is set");
        } else {
            sendToClient(fd, ":server 332 " + client->getNickname() +
                                  " " + channelName + " :" + channel.getTopic());
        }
        return;
    }

    // Установка новой темы
    // Проверка на +t (topic restricted)
    if (channel.isTopicRestricted() && !channel.isOperator(fd)) {
        sendToClient(fd, ":server 482 " + client->getNickname() + " " +
                              channelName + " :You're not channel operator");
        return;
    }

    // Составляем тему из всех параметров после 2-го
    std::string topic = params[2];
    for (size_t i = 3; i < params.size(); ++i) {
        topic += " " + params[i];
    }
    if (topic[0] == ':') topic = topic.substr(1);  // убираем двоеточие

    channel.setTopic(topic);

    // Сообщение всем участникам канала
    const std::set<int>& members = channel.getClients();
    std::string topicMsg = ":" + client->getNickname() +
                           " TOPIC " + channelName + " :" + topic;
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it) {
        sendToClient(*it, topicMsg);
    }

    std::cout << "Client fd " << fd << " set topic for channel " 
              << channelName << " to: " << topic << std::endl;
}