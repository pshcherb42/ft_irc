NAME=ircserv
CC=c++
CFLAGS=-Wall -Wextra -Werror -std=c++98
OBJ_DIR = obj

SRC=	main.cpp \
		Server.cpp \
		Client.cpp \
		Channel.cpp \
		commands/Auth.cpp \
		commands/channel/Join.cpp \
		commands/channel/Part.cpp \
		commands/channel/Mode.cpp \
		commands/channel/Topic.cpp \
		commands/channel/Invite.cpp \
		commands/channel/Kick.cpp \
		commands/Message.cpp \

OBJ=$(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ) Makefile Server.cpp Client.cpp  Channel.cpp
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re