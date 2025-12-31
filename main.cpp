#include <poll.h>
#include <fcntl.h>
#include <cstring>
#include <iostream> // for cout and cin
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> 
#include <unistd.h>
#include <vector>

int main() {
    
    // creating the server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); // (family, type, protocol) returns -1 on error
    if (serverSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }
    // Set socket to non-blocking mode
    if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "Error setting non-blocking" << std::endl;
        close(serverSocket);
        return 1;
    }
    // defining server address
    sockaddr_in serverAddress; // data type for addresses
    std::memset(&serverAddress, 0, sizeof(serverAddress)); // Clear structure
    serverAddress.sin_family = AF_INET; // IPv4 protocol
    serverAddress.sin_port = htons(8080); // function to convert machine byte order to network byte order
    serverAddress.sin_addr.s_addr = INADDR_ANY; // No bind socket to any particular ip, yes listen to all availible ips
    // bind socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        close(serverSocket);
        return 1;
    }
    // Listen for incoming connections
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket);
        return 1;
    }
    // converts an unconnected socket into a passive socket, indicating that the kernel should accept incoming connection requests directed to this socket. (sockfd, maximum number of connections the kernel should queue for this socket. )
    std::cout << "Server listening on port 8080..." << std::endl;
    // one poll() for everything read/write & listen etc. multiple clients, i/o non-blocking
    
    // Initialize poll array
    std::vector<struct pollfd> fds;

    // Add server socket to poll array
    struct pollfd serverPollFd;
    serverPollFd.fd = serverSocket; // we read from
    serverPollFd.events = POLLIN; // Watch for incoming connections
    serverPollFd.revents = 0; // output parameter, event that accured
    fds.push_back(serverPollFd); // llenamos el vector

    // Main server loop
    while (true) {
        // Call poll() - waits for events on any file descriptor
        // -1 means wait indefinitely
        int pollCount = poll(&fds[0], fds.size(), -1); // this one is just to see beforehand if something happens and if not just break the loop
        
        if (pollCount == -1) {
            std::cerr << "Error in poll()" << std::endl;
            break;
        }

        // we run through fds vector filled with pollfd structs
        for (size_t i = 0; i < fds.size(); i++) {
            // Check if this fd has an event
            if (fds[i].revents == 0) {
                continue; // No event on this fd
            }

            // Check for errors
            if (fds[i].revents & POLLERR || fds[i].revents & POLLHUP || fds[i].revents & POLLNVAL) {
                std::cerr << "Error on fd " << fds[i].fd << std::endl;
                close(fds[i].fd);
                fds.erase(fds.begin() + i);
                i--; // go back one 
                continue;
            }

            // Check if it's the server socket (new connection)
            if (fds[i].fd == serverSocket && (fds[i].revents & POLLIN)) {
                // Accept new client
                sockaddr_in clientAddress;
                socklen_t clientLen = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLen);
                
                if (clientSocket == -1) {
                    std::cerr << "Error accepting client" << std::endl;
                    continue;
                }

                // Set client socket to non-blocking
                if (fcntl(clientSocket, F_SETFL, O_NONBLOCK) == -1) {
                    std::cerr << "Error setting client non-blocking" << std::endl;
                    close(clientSocket);
                    continue;
                }

                // Add client to poll array
                struct pollfd clientPollFd;
                clientPollFd.fd = clientSocket;
                clientPollFd.events = POLLIN; // Watch for incoming data
                clientPollFd.revents = 0;
                fds.push_back(clientPollFd);

                std::cout << "New client connected: fd " << clientSocket << std::endl;
            }
            // Handle client data
            else if (fds[i].revents & POLLIN) {
                char buffer[1024];
                std::memset(buffer, 0, sizeof(buffer)); // set all to zero
                
                ssize_t bytesRead = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesRead <= 0) {
                    // Client disconnected or error
                    if (bytesRead == 0) {
                        std::cout << "Client disconnected: fd " << fds[i].fd << std::endl;
                    } else {
                        std::cerr << "Error reading from client: fd " << fds[i].fd << std::endl;
                    }
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    i--; // Adjust index after erase
                } else {
                    // Data received
                    std::cout << "Received from fd " << fds[i].fd << ": " << buffer;
                    
                    // Echo back to client (example)
                    send(fds[i].fd, buffer, bytesRead, 0);
                }
            }
        }
    }

        // Cleanup
    for (size_t i = 0; i < fds.size(); i++) {
        close(fds[i].fd);
    }
}

//Events to Watch

/*POLLIN - Data available to read
POLLOUT - Ready to write (use when sending data)
POLLERR - Error condition
POLLHUP - Hang up (client disconnected)
POLLNVAL - Invalid request

The Poll Loop Flow

Call poll() - blocks until an event occurs
Loop through all file descriptors
Check revents to see what happened
Handle server socket (new connections)
Handle client sockets (data or disconnection)*/