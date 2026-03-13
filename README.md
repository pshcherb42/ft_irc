*This project has been created as part of the 42 curriculum by akreise and pshcherb*

# Description

ft_irc is a fully functional IRC server written in C++98, following the RFC 1459 specification guidelines. It allows multiple clients to connect simultaneously, register with a nickname and username, join channels, send messages, and be managed by channel operators. All of this is implemented without using threads or fork, running in a single process with non-blocking I/O using poll().

# Instructions

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

try PRIVMSG #testchannel :Hello, everniyane!
NOTICE #testchannel :Hello, everniyane!

*with irssi - put in terminal irssi
then /connect 127.0.0.1 6667 secret123

to kill the process - fuser -k 6669/tcp
(to kill:  lsof -i :6667
kill nomer)

# errors

For the *"No such channel" message* — that's irssi trying to auto-join its saved channels. It's not your server printing that, it's irssi's UI showing the 403 error your server correctly returns. You can ignore this entirely.

# Resources

Internet Relay Chat Protocol Documentation
https://www.rfc-editor.org/rfc/rfc1459.html#section-1.1

poll() - documentation page
https://pubs.opengroup.org/onlinepubs/009696799/functions/poll.html

Ejemplo: Utilización de señales con API de socket de bloqueo
https://www.ibm.com/docs/es/i/7.5.0?topic=designs-example-using-signals-blocking-socket-apis

IRC tutorial
https://medium.com/@afatir.ahmedfatir/small-irc-server-ft-irc-42-network-7cee848de6f9

Modelo TCP/IP
https://es.wikipedia.org/wiki/Modelo_TCP/IP

Internet Relay Chat
https://es.wikipedia.org/wiki/Internet_Relay_Chat

Choosing irc client
https://libera.chat/guides/clients

FT_IRC : Channels and Command Management
https://medium.com/@mohamedsarda/ft-irc-channels-and-command-management-ff1ff3758a0b

Claude.ai - to sructure the defense presentation.

ChatGPT - to determine work division strategy. We made our own division work strategy and consulted chat if it was the most optimal, it drew out attention that it was not the most optimal and offered a better tast separation which we did.

ChatGPT - loop not entering debug. We had a problem with \r\n inside commands parsing. The thing is that initialy we were anaware that irssi needed \r separator to consider commands full and we could find the problem. Finally we asked chat and he pointed to this issue. 

