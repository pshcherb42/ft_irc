// Server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <poll.h>

class Client; // Forward declaration

class Server {
private:
    int _port;
    std::string _password;
    int _serverSocket;
    std::vector<struct pollfd> _fds;
    std::map<int, Client*> _clients;

public:
    Server(int port, const std::string& password);
    ~Server();
    
    void start();
    
private:
    void setupSocket();
    void acceptNewClient();
    void handleClientData(int fd);
    void removeClient(int fd);
    void processCommand(int fd, const std::string& command);
    
    // Command handlers
    void cmdPass(int fd, const std::vector<std::string>& params);
    void cmdNick(int fd, const std::vector<std::string>& params);
    void cmdUser(int fd, const std::vector<std::string>& params);
    
    // Utilities
    void sendToClient(int fd, const std::string& message);
    std::vector<std::string> split(const std::string& str);
};

#endif