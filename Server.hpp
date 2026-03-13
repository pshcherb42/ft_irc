// Server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <poll.h>
#include "Channel.hpp"

class Client; // Forward declaration — говорим, что класс Client существует, чтобы использовать указатели

class Server 
{
private:
    Server();
    Server(const Server&);
    Server& operator=(const Server&);
    int _port; // Порт, на котором слушает сервер
    std::string _password; // Пароль для подключения (PASS)
    int _serverSocket; // Сокет сервера
    std::vector<struct pollfd> _fds; // Вектор для poll() всех сокетов (сервер + клиенты)
    std::map<int, Client*> _clients; // Словарь: fd -> объект Client
    std::map<std::string, Channel> _channels;
public:
    Server(int port, const std::string& password);
    ~Server();
    
    void start(); //основной цикл сервера
    
private:
    void setupSocket(); // Создание и настройка серверного сокета
    void acceptNewClient(); // Принятие нового подключения
    void handleClientData(int fd); // Чтение данных от клиента
    void removeClient(int fd); // Удаление клиента (отключение)
    void processCommand(int fd, const std::string& command); // Разбор и выполнение команды
    
    // Command handlers
    void cmdPass(int fd, const std::vector<std::string>& params);
    void cmdNick(int fd, const std::vector<std::string>& params);
    void cmdUser(int fd, const std::vector<std::string>& params);
    void cmdJoin(int fd, const std::vector<std::string>& params);
    void cmdPrivmsg(int fd, const std::vector<std::string>& params);
    void cmdPart(int fd, const std::vector<std::string>& params);
    void cmdMode(int fd, const std::vector<std::string>& params);
    void cmdInvite(int fd, const std::vector<std::string>& params);
    void cmdKick(int fd, const std::vector<std::string>& params);
    void cmdTopic(int fd, const std::vector<std::string>& params);
    void cmdNotice(int fd, const std::vector<std::string>&params);
    
    // Utilities
    void sendToClient(int fd, const std::string& message); // Отправка сообщения клиенту
    std::vector<std::string> split(const std::string& str); // Разделение строки на слова по пробелу
    bool isValidChannelName(const std::string& name); // check valid channel name
};

#endif