// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int _fd;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    std::string _buffer;      // Accumulates partial data
    bool _authenticated;      // PASS received and correct?
    bool _registered;         // NICK + USER both received?

public:
    Client(int fd);
    ~Client();
    
    // Getters
    int getFd() const ;
    std::string getNickname() const ;
    std::string getUsername() const ;
    std::string& getBuffer() ;
    bool isAuthenticated() const ;
    bool isRegistered() const ;
    
    // Setters
    void setNickname(const std::string& nick) ;
    void setUsername(const std::string& user) ;
    void setRealname(const std::string& real) ;
    void setAuthenticated(bool auth) ;
    void setRegistered(bool reg) ;
    
    void appendToBuffer(const std::string& data) ;
};

#endif