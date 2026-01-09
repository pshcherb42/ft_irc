# ft_irc

Our own IRC server, we will use irssi client to connect 

# how to use

compile using make
./ircserv 6667 secret123
 and on other terminal : nc 127.0.0.1 6667

 now client and server should connect

 on client side write: PASS secret123
 then: NICK yournick
 then: USER 0 * :yourusername

 This should grant you access to the server and on a client side you will see something like :server 001 alice :Welcome to the IRC Network
:server 002 alice :Your host is server
:server 003 alice :This server was created today
:server 004 alice server 1.0 o o

# what is done

Socket setup with non-blocking I/O

Poll loop handling multiple clients

Command parsing(handling "\n") -- should be "\r\n" -- but for testing only works with "\n", we should remember to change it later

Authentification (PASS,NICK,USER)

# possible next steps

JOIN - create/join channels

PRIVMSG -  Send messages to channels/user

Operator Commands(KICK, INVITE, TOPIC, MODE)
