// Server.cpp
#include "Server.hpp"
#include "Client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>  // For std::toupper

Server::Server(int port, const std::string& password) 
    : _port(port), _password(password), _serverSocket(-1) {
    setupSocket(); // Настраиваем сокет при создании сервера
}

Server::~Server() {
    // Clean up all clients
    for (std::map<int, Client*>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        delete it->second;
    }
    
    // Close all file descriptors
    for (size_t i = 0; i < _fds.size(); i++) {
        close(_fds[i].fd);
    }
}

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

void Server::setupSocket() {
    //Create socket
    _serverSocket = socket(AF_INET,SOCK_STREAM, 0); // TCP-сокет
    if (_serverSocket == -1) {
        throw std::runtime_error("Error creating socket");;
    }
    //Set to non-blocking
    if(fcntl(_serverSocket, F_GETFL, O_NONBLOCK) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Error setting non-blocking");
    }
    //Set SO_REUSEADDR option - Можно переподключать порт без ожидания
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Error setting socket options");
    }
    //Bind to address
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_port);  // Use _port from constructor!
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // No need for htonl with INADDR_ANY

    // Привязка к порту
    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Error binding socket");
    }

    //Listen - Начало прослушивания
    if (listen(_serverSocket, 10) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Error listening on socket");
    }

    //Add server socket to _fds vector -  Добавляем серверный сокет в poll
    struct pollfd serverPollFd;
    serverPollFd.fd = _serverSocket;
    serverPollFd.events = POLLIN;  // Watch for incoming connections
    serverPollFd.revents = 0;
    _fds.push_back(serverPollFd);
    
    std::cout << "Server listening on port " << _port << std::endl;

}

void Server::start() {
    std::cout << "IRC Server started. Waiting for connections..." << std::endl;
    
    while (true) {
        // Call poll() - wait for events - Ждём события на любом сокете
        int pollCount = poll(&_fds[0], _fds.size(), -1);
        
        if (pollCount == -1) {
            throw std::runtime_error("Error in poll()");
        }
        
        // Check all file descriptors for events
        for (size_t i = 0; i < _fds.size(); i++) {
            // Skip if no event
            if (_fds[i].revents == 0) {
                continue;  // Нет событий
            }
            
            // Check for errors
            if (_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                std::cerr << "Error on fd " << _fds[i].fd << std::endl;
                removeClient(_fds[i].fd);
                i--;  // Adjust index after removal
                continue;
            }
            
            // Is it the server socket? (new connection)
            if (_fds[i].fd == _serverSocket && (_fds[i].revents & POLLIN)) {
                acceptNewClient();
            }
            // Is it a client socket? (incoming data)
            else if (_fds[i].revents & POLLIN) {
                handleClientData(_fds[i].fd);
            }
        }
    }
}

void Server::acceptNewClient() //Создаёт новый объект Client для каждого подключения
{
    // accept connection
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int new_socket = accept(_serverSocket, (struct sockaddr*)&address,
                  &addrlen); // Принимаем нового клиента

    if (new_socket == -1) {
        std::cerr << "Error accepting client" << std::endl;
        return;  // Don't crash, just continue
    }
    // set client socket to non blocking 
    if(fcntl(new_socket, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "Error setting client non-blocking" << std::endl;
        close(new_socket);
        return;
    }
    // create a new client object
    Client* new_client = new Client(new_socket);
    // add to _clients map
    _clients[new_socket] = new_client;
    // add to _fds vector
    struct pollfd clientPollFd;
    clientPollFd.fd = new_socket;
    clientPollFd.events = POLLIN;  // Watch for incoming data
    clientPollFd.revents = 0;
    _fds.push_back(clientPollFd);
    // print message
    std::cout << "New client connected: fd " << new_socket << std::endl;
}

void Server::removeClient(int fd) {
    // find and remove from _fds vector
    for (size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd) {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
    // find and delete from _clients map(delete to free memory)
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end()) {
        delete it->second;
        _clients.erase(it);
    }
    // close the file descriptor
    close(fd);
    // print message
    std::cout << "Client disconnected: fd " << fd << std::endl;
}

void Server::handleClientData(int fd) {
    // 1. Receive data into buffer
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    //std::cout << "DEBUG: recv() returned " << bytesRead << " bytes from fd " << fd << std::endl;
    
    // 2. Check if recv failed (disconnect/error)
    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "Client disconnected: fd " << fd << std::endl;
        } else {
            std::cerr << "Error reading from client: fd " << fd << std::endl;
        }
        removeClient(fd);
        return;
    }

       // std::cout << "DEBUG: Received data: [" << buffer << "]" << std::endl;

    
    // 3. Append received data to client's buffer
    Client* client = _clients[fd];
    client->getBuffer().append(buffer, bytesRead);  // Append exactly what we received
    
    // 4. Extract complete commands (ending with \r\n)
    std::string& clientBuffer = client->getBuffer();
    //std::cout << "DEBUG: Client buffer now: [" << clientBuffer << "]" << std::endl;
    size_t pos;
    
    while ((pos = clientBuffer.find("\n")) != std::string::npos) {
        // Extract one complete command
        std::string command = clientBuffer.substr(0, pos);
        clientBuffer.erase(0, pos + 2);  // Remove command + \r\n

        //std::cout << "DEBUG: Extracted command: [" << command << "]" << std::endl;
        
        // 5. Process this command
        if (!command.empty()) {
            processCommand(fd, command);
        }
    }
    // Incomplete data stays in buffer for next recv()
}

// Send message to a client
void Server::sendToClient(int fd, const std::string& message) {
    std::string msg = message + "\r\n";
    send(fd, msg.c_str(), msg.length(), 0);
}

// Split string by spaces
std::vector<std::string> Server::split(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string word;
    
    while (ss >> word) {
        result.push_back(word);
    }
    
    return result;
}

void Server::processCommand(int fd, const std::string& command) {
    std::cout << "Command from fd " << fd << ": " << command << std::endl;
    
    // 1. Split the command string by spaces
    std::vector<std::string> params = split(command);
    
    if (params.empty()) {
        return;  // Empty command, ignore
    }
    
    // 2. Convert command name to uppercase
    std::string cmd = params[0];
    for (size_t i = 0; i < cmd.length(); i++) {
        cmd[i] = std::toupper(cmd[i]);
    }
    
    // 3. Call the appropriate handler
    if (cmd == "PASS") {
        cmdPass(fd, params);
    }
    else if (cmd == "NICK") {
        cmdNick(fd, params);
    }
    else if (cmd == "USER") {
        cmdUser(fd, params);
    }
    else if (cmd == "JOIN") {
        cmdJoin(fd, params);
    }
    else if (cmd == "PRIVMSG") {
        cmdPrivmsg(fd, params);
    }    
    else {
        // Unknown command
        sendToClient(fd, ":server 421 * " + cmd + " :Unknown command");
    }
}

void Server::cmdPass(int fd, const std::vector<std::string>& params) {
    Client* client = _clients[fd];
    
    // Check if enough parameters
    if (params.size() < 2) {
        sendToClient(fd, ":server 461 * PASS :Not enough parameters");
        return;
    }
    
    // Check password
    if (params[1] == _password) {
        client->setAuthenticated(true);
        std::cout << "Client fd " << fd << " authenticated" << std::endl;
    } else {
        sendToClient(fd, ":server 464 * :Password incorrect");
        removeClient(fd);  // Wrong password = disconnect
    }
}

void Server::cmdNick(int fd, const std::vector<std::string>& params) {
    Client* client = _clients[fd];
    
    // 1. Check if client is authenticated
    if (!client->isAuthenticated()) {
        sendToClient(fd, ":server 451 * :You have not registered");
        return;
    }
    
    // 2. Check if enough parameters
    if (params.size() < 2) {
        sendToClient(fd, ":server 431 * :No nickname given");
        return;
    }
    
    std::string newNick = params[1];
    
    // 3. Check if nickname is already taken by another client
    for (std::map<int, Client*>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        if (it->first != fd && it->second->getNickname() == newNick) {
            sendToClient(fd, ":server 433 * " + newNick + " :Nickname is already in use");
            return;
        }
    }
    
    // 4. Set the nickname
    client->setNickname(newNick);
    std::cout << "Client fd " << fd << " set nickname: " << newNick << std::endl;
    
    // 5. If both nickname AND username are set, mark as registered
    if (!client->getUsername().empty() && !client->isRegistered()) {
        client->setRegistered(true);
        sendToClient(fd, ":server 001 " + newNick + " :Welcome to the IRC Network");
        std::cout << "Client fd " << fd << " is now fully registered" << std::endl;
    }
}

void Server::cmdUser(int fd, const std::vector<std::string>& params) {
    Client* client = _clients[fd];  // FIX: You forgot to initialize!
    
    // 1. Check if authenticated
    if (!client->isAuthenticated()) {
        sendToClient(fd, ":server 451 * :You have not registered");
        return;
    }
    
    // 2. Check if enough parameters (USER username hostname servername :realname)
    if (params.size() < 5) {
        sendToClient(fd, ":server 461 * USER :Not enough parameters");
        return;
    }
    
    // 3. Set username (params[1])
    std::string username = params[1];
    client->setUsername(username);
    std::cout << "Client fd " << fd << " set username: " << username << std::endl;
    
    // 4. Set realname (params[4] onwards, removing leading ':')
    std::string realname = params[4];
    if (realname[0] == ':') {
        realname = realname.substr(1);  // Remove the ':'
    }
    
    // If there are more parameters, join them (realname can have spaces)
    for (size_t i = 5; i < params.size(); i++) {
        realname += " " + params[i];
    }
    
    client->setRealname(realname);
    std::cout << "Client fd " << fd << " set realname: " << realname << std::endl;
    
    // 5. If both nickname AND username are set, mark as registered
    if (!client->getNickname().empty() && !client->isRegistered()) {
        client->setRegistered(true);
        std::string nick = client->getNickname();
        sendToClient(fd, ":server 001 " + nick + " :Welcome to the IRC Network");
        sendToClient(fd, ":server 002 " + nick + " :Your host is server");
        sendToClient(fd, ":server 003 " + nick + " :This server was created today");
        sendToClient(fd, ":server 004 " + nick + " server 1.0 o o");
        std::cout << "Client fd " << fd << " is now fully registered" << std::endl;
    }
}

void Server::cmdJoin(int fd, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        sendToClient(fd, ":server 461 * JOIN :Not enough parameters");
        return;
    }

    if (_clients.find(fd) == _clients.end()) {
        return;
    }

    Client* client = _clients[fd];

    if (!client->isRegistered()) {
        sendToClient(fd, ":server 451 * :You have not registered");
        return;
    }

    std::string channelName = params[1];

    if (channelName.empty() || channelName[0] != '#') {
        sendToClient(fd,
            ":server 403 " + channelName + " :No such channel");
        return;
    }

    // найти или создать канал
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        _channels.insert(std::make_pair(channelName, Channel(channelName)));
        it = _channels.find(channelName);
    }

    Channel& channel = it->second;

    if (channel.hasClient(fd)) {
        return;
    }

    channel.addClient(fd);

    std::string joinMsg =
        ":" + client->getNickname() + " JOIN " + channelName;

    const std::set<int>& members = channel.getClients();
    for (std::set<int>::const_iterator m = members.begin();
         m != members.end(); ++m) {
        sendToClient(*m, joinMsg);
    }

    std::cout << "Client fd " << fd
              << " joined channel " << channelName << std::endl;
}

void Server::cmdPrivmsg(int fd, const std::vector<std::string>& params) {
    // 1. Проверяем регистрацию клиента
    if (_clients.find(fd) == _clients.end())
        return;

    Client* sender = _clients[fd];
    if (!sender->isRegistered()) {
        sendToClient(fd, ":server 451 * :You have not registered");
        return;
    }

    // 2. Проверяем, что есть хотя бы 2 параметра: получатель и сообщение
    if (params.size() < 3) {
        sendToClient(fd, ":server 461 * PRIVMSG :Not enough parameters");
        return;
    }

    std::string target = params[1]; // #channel или ник пользователя
    std::string message;

    // Собираем всё остальное как текст сообщения
    message = params[2];
    for (size_t i = 3; i < params.size(); i++) {
        message += " " + params[i];
    }

    // Убираем ведущий ':', если есть
    if (!message.empty() && message[0] == ':') {
        message = message.substr(1);
    }

    // 3. Если target начинается с '#', это канал
    if (target[0] == '#') {
        std::map<std::string, Channel>::iterator it = _channels.find(target);
        if (it == _channels.end()) {
            sendToClient(fd, ":server 403 " + target + " :No such channel");
            return;
        }

        Channel& channel = it->second;
        if (!channel.hasClient(fd)) {
            sendToClient(fd, ":server 442 " + target + " :You're not on that channel");
            return;
        }

        // Отправляем ВСЕМ клиентам канала
        std::string fullMsg = ":" + sender->getNickname() + " PRIVMSG " + target + " :" + message;
        const std::set<int>& members = channel.getClients();
        for (std::set<int>::const_iterator it2 = members.begin(); it2 != members.end(); ++it2) {
            sendToClient(*it2, fullMsg);
        }
    } 
    else {
        // 4. Если target — ник пользователя
        bool found = false;
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->second->getNickname() == target) {
                std::string fullMsg = ":" + sender->getNickname() + " PRIVMSG " + target + " :" + message;
                sendToClient(it->first, fullMsg);
                found = true;
                break;
            }
        }
        if (!found) {
            sendToClient(fd, ":server 401 " + target + " :No such nick");
        }
    }
}

