// Client.cpp
#include "Client.hpp"

Client::Client(int fd) 
    : _fd(fd), _authenticated(false), _registered(false) {
    // Constructor: initialize with fd, everything else empty/false
}

Client::~Client() {
    // Destructor: nothing to clean up (Server closes the fd)
}

// Getters — возвращают приватные поля
    int Client::getFd() const { return _fd; }
    std::string Client::getNickname() const { return _nickname; }
    std::string Client::getUsername() const { return _username; }
    std::string& Client::getBuffer() { return _buffer; } // возвращаем ссылку
    std::string Client::getPrefix() const
    {
        std::string user = _username.empty() ? "user" : _username;
        std::string host = "localhost";
    
        return ":" + _nickname + "!" + user + "@" + host;
    }
    bool Client::isAuthenticated() const { return _authenticated; }
    bool Client::isRegistered() const { return _registered; }
    
    // Setters - задаём новые значения
    void Client::setNickname(const std::string& nick) { _nickname = nick; }
    void Client::setUsername(const std::string& user) { _username = user; }
    void Client::setRealname(const std::string& real) { _realname = real; }
    void Client::setAuthenticated(bool auth) { _authenticated = auth; }
    void Client::setRegistered(bool reg) { _registered = reg; }
    
    // Добавляем данные в буфер (для неполных recv)
    void Client::appendToBuffer(const std::string& data) { _buffer += data; }