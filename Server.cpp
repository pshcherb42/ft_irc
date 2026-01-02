// Server.cpp
#include "Server.hpp"
#include "Client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>

Server::Server(int port, const std::string& password) 
    : _port(port), _password(password), _serverSocket(-1) {
    setupSocket();
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

void Server::setupSocket() {
    //Create socket
    //Set to non-blocking
    //Set SO_REUSEADDR option
    //Bind to address
    //Listen
    //Add server socket to _fds vector
}