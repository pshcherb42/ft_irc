# ft_irc

Our own IRC server, we will use irssi client to connect 

# how to use

compile using make
./ircserv 6667 secret123
 and on other terminal : nc 127.0.0.1 6667

 now client and server should connect

 on client side write: PASS secret123
 then: NICK yournick
 then: USER 0 * : yourusername

 This should grant you access to the server and on a client side you will see something like :server 001 alice :Welcome to the IRC Network
:server 002 alice :Your host is server
:server 003 alice :This server was created today
:server 004 alice server 1.0 o o

After try JOIN #testchannel

try PRIVMSG #testchannel :Hello, everyne!

# what is done

Socket setup with non-blocking I/O

Poll loop handling multiple clients

Command parsing(handling "\n") -- should be "\r\n" -- but for testing only works with "\n", we should remember to change it later

Authentification (PASS,NICK,USER)

JOIN - create/join channels

PRIVMSG -  Send messages to channels/user

Operator Commands(PART, MODE, KICK, INVITE, TOPIC)

# possible next steps

Лимит на количество каналов, в которых может быть клиент.
Уведомления о JOIN / PART / KICK всем участникам канала.

Подключить irssi

проверить если неправильно ввел пароль при пасс то при повторном вводе выходит из программы
после mode #channel smth выдает сообщение и делает доп пропуск строки

Потом проерить что есть ридми файл для проверки
Проверить что все классы сделаны по каноникал форм (если надо)
