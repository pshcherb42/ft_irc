NAME=ircserv
CC=c++
CFLAGS=-Wall -Wextra -Werror -std=c++98

SRC=	main.cpp \
		Server.cpp \
		Client.cpp \

OBJ=$(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ) Makefile Server.cpp Client.cpp 
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re