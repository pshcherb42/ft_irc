// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    Client();
    Client(const Client&);
    Client& operator=(const Client&);
    int _fd;                  //файловый дескриптор сокета
    std::string _nickname;    //данные аутентификации
    std::string _username;    
    std::string _realname;

    std::string _buffer;      // Accumulates partial data(буфер, куда накапливаются частичные данные из recv() (для non-blocking I/O))
    bool _authenticated;      // PASS received and correct?
    bool _registered;         // NICK + USER both received?

public:
    Client(int fd);
    ~Client();
    
    // Getters — методы для получения данных
    int getFd() const ;
    std::string getNickname() const ;
    std::string getUsername() const ;
    std::string& getBuffer() ; // возвращаем ссылку, чтобы можно было изменять
    std::string getPrefix() const;
    bool isAuthenticated() const ;
    bool isRegistered() const ;
    
    // Setters — методы для изменения данных
    void setNickname(const std::string& nick) ;
    void setUsername(const std::string& user) ;
    void setRealname(const std::string& real) ;
    void setAuthenticated(bool auth) ;
    void setRegistered(bool reg) ;
    
    // Добавляет данные в буфер (используется для частичных пакетов recv())
    void appendToBuffer(const std::string& data) ;//для накопления данных
};

#endif