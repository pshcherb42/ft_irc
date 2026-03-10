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

*with irssi - put in terminal irssi
then /connect 127.0.0.1 6667 secret123

уничтожить процесс - fuser -k 6669/tcp
(to kill:  lsof -i :6667
kill nomer)

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

# errors

For the *"No such channel" message* — that's irssi trying to auto-join its saved channels. It's not your server printing that, it's irssi's UI showing the 403 error your server correctly returns. You can ignore this entirely.

# changes 10 march

change get fcntl a set fcntl inside setupSocket in server.cpp
adde ping pong and cap and quit, who inside proccessComand in server.cpp


Да, это **нормальное поведение `irssi`**: после `KICK` клиент может оставить окно канала открытым, даже если пользователя уже выгнали и он больше не состоит в канале. `irssi` вообще любит отделять “окно интерфейса” от “членства в канале”. В его документации и обсуждениях видно, что окна каналов могут жить своей жизнью, а отдельные настройки управляют автозакрытием окон в похожих ситуациях; при этом FAQ отдельно обсуждает поведение после `KICK`, не предполагая, что окно обязано исчезнуть автоматически. ([Irssi::Help][1])

То, что пользователь **не может больше писать в канал и ничего там делать**, как раз хороший признак: сервер, скорее всего, правильно удалил его из канала. Подозрительно было бы обратное: если после `KICK` окно осталось и клиент всё ещё мог бы слать сообщения как участник канала. ([Irssi::Help][1])

Тут важно различать две вещи. После `KICK` сервер должен корректно разослать событие `KICK`, а `irssi` уже сам решает, как это показать в UI. Само наличие окна не доказывает баг сервера. Баг был бы, если:

* kicked-клиент всё ещё числится в `NAMES`,
* может писать в канал,
* или `irssi` не показывает, что его выгнали. ([irssi.org][2])

Проверяй так:

1. у выгнанного клиента сделай `/names #test`,
2. попробуй написать сообщение в этом окне,
3. посмотри rawlog и убедись, что пришёл нормальный `KICK`.

Если после `KICK` он не в списке и писать не может, то с большой вероятностью всё ок, а окно просто осталось как вкладка. У `irssi` это не редкость, потому что древний терминальный софт считает, что закрывать окна за пользователя было бы слишком человечно. ([Carina][3])


Если ты используешь irssi, список всех открытых окон можно посмотреть командой:

/window list

Как переключаться между окнами

Самый быстрый способ:

Alt + 1
Alt + 2
Alt + 3