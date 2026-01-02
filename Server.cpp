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

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

void Server::setupSocket() {
    //Create socket
    _serverSocket = socket(AF_INET,SOCK_STREAM, 0);
    if (_serverSocket == -1) {
        throw std::runtime_error("Error creating socket");;
    }
    //Set to non-blocking
    if(fcntl(_serverSocket, F_GETFL, O_NONBLOCK) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Error setting non-blocking");
    }
    //Set SO_REUSEADDR option
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

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Error binding socket");
    }

    //Listen
    if (listen(_serverSocket, 10) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Error listening on socket");
    }

    //Add server socket to _fds vector
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
        // Call poll() - wait for events
        int pollCount = poll(&_fds[0], _fds.size(), -1);
        
        if (pollCount == -1) {
            throw std::runtime_error("Error in poll()");
        }
        
        // Check all file descriptors for events
        for (size_t i = 0; i < _fds.size(); i++) {
            // Skip if no event
            if (_fds[i].revents == 0) {
                continue;
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

void Server::acceptNewClient() {
    // accept connection
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int new_socket = accept(_serverSocket, (struct sockaddr*)&address,
                  &addrlen);

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