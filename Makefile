NAME = ircserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++17 -I./includes
RM = rm -f

SRCS = main.cpp \
       srcs/server.cpp \
       srcs/logger.cpp \
       srcs/client.cpp \
       srcs/channel.cpp \
	   srcs/InputParser.cpp

OBJ_DIR	= obj
OBJS	= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS 	= includes/server.hpp includes/logger.hpp includes/client.hpp includes/channel.hpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "\033[0;32mServer compiled. Run with: ./ircserv <port> <password>\033[0m"

$(OBJ_DIR)/%.o: %.cpp $(DEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re


