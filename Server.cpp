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

Server::~Server() 
{
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

void Server::setupSocket() 
{
    //Create socket
    _serverSocket = socket(AF_INET,SOCK_STREAM, 0); // TCP-сокет
    if (_serverSocket == -1) {
        throw std::runtime_error("Error creating socket");;
    }
    //Set to non-blocking
    if(fcntl(_serverSocket, F_SETFL, O_NONBLOCK) == -1) {
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

void Server::start() 
{
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

void Server::removeClient(int fd) 
{
    // find and remove from _fds vector
    for (size_t i = 0; i < _fds.size(); i++) 
    {
        if (_fds[i].fd == fd) 
        {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
    // find and delete from _clients map(delete to free memory)
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end()) 
    {
        delete it->second;
        _clients.erase(it);
    }
    // close the file descriptor
    close(fd);
    // print message
    std::cout << "Client disconnected: fd " << fd << std::endl;
}

void Server::handleClientData(int fd) 
{
    // 1. Receive data into buffer
    char buffer[1024]; // in original irc protoco is 512 bytes
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    // 2. Check if recv failed (disconnect/error)
    if (bytesRead <= 0)
    {
        if (bytesRead == 0)
            std::cout << "Client disconnected: fd " << fd << std::endl;
        else
            std::cerr << "Error reading from client: fd " << fd << std::endl;
        removeClient(fd);
        return;
    }
    // 3. Append received data to client's buffer
    Client* client = _clients[fd];
    client->getBuffer().append(buffer, bytesRead);  // Append exactly what we received
    
    // 4. Extract complete commands (ending with \r\n)
    std::string& clientBuffer = client->getBuffer();
    size_t pos;

    while ((pos = clientBuffer.find('\n')) != std::string::npos)
    {
        std::string command = clientBuffer.substr(0, pos);

    // удаляем \r если это был CRLF
        if (!command.empty() && command[command.size() - 1] == '\r')
            command.erase(command.size() - 1);

        clientBuffer.erase(0, pos + 1);

        if (!command.empty())
            processCommand(fd, command);
    }
    
    // Incomplete data stays in buffer for next recv()
}

// Send message to a client
void Server::sendToClient(int fd, const std::string& message) 
{
    std::string msg = message + "\r\n";
    send(fd, msg.c_str(), msg.length(), 0);
}

// Split string by spaces
std::vector<std::string> Server::split(const std::string& str) 
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string word;
    
    while (ss >> word)
        result.push_back(word);
    return (result);
}

void Server::processCommand(int fd, const std::string& command) 
{
    std::cout << "Command from fd " << fd << ": " << command << std::endl;
    
    // 1. Split the command string by spaces
    std::vector<std::string> params = split(command);
    
    if (params.empty())
        return;  // Empty command, ignore
    
    // 2. Convert command name to uppercase
    std::string cmd = params[0];
    for (size_t i = 0; i < cmd.length(); i++)
        cmd[i] = std::toupper(cmd[i]);
    // In your dispatcher, before the "not registered" check:
    if (cmd == "CAP")
        return; // silently ignore
    
    if (cmd == "PING") {
        std::string token = params.size() > 1 ? params[1] : "ping";
        sendToClient(fd, ":server PONG server :" + token);
        return;
    }

    if (cmd == "QUIT"){
        removeClient(fd);
        return;
    }
    // estos dos los ignoramos porque el subject no los pide
    if (cmd == "WHO") // deberia dar detalles tecnicos de los usuarios
        return;

    if (cmd == "WHOIS") // dice en que canales esta esa persona
        return;
    // 3. Call the appropriate handler
    if (cmd == "PASS")
        cmdPass(fd, params);
    else if (cmd == "NICK")
        cmdNick(fd, params);
    else if (cmd == "USER")
        cmdUser(fd, params);
    else if (cmd == "JOIN")
        cmdJoin(fd, params);
    else if (cmd == "PRIVMSG")
        cmdPrivmsg(fd, params);
    else if (cmd == "PART")
        cmdPart(fd, params);
    else if (cmd == "MODE")
        cmdMode(fd, params);
    else if (cmd == "INVITE")
        cmdInvite(fd, params);
    else if (cmd == "KICK")
        cmdKick(fd, params);
    else if (cmd == "TOPIC")
        cmdTopic(fd, params);
    else if (cmd == "NOTICE")
        cmdNotice(fd, params);
    else {
        // Unknown command
        sendToClient(fd, ":server 421 * " + cmd + " :Unknown command");
    }
}