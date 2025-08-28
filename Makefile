NAME = ircserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -I./includes
RM = rm -f

SRCS = main.cpp \
       srcs/server.cpp \
	   srcs/logger.cpp \

OBJS = $(SRCS:.cpp=.o)
DEPS = includes/server.hpp includes/Logger.hpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "\033[0;32mServer compiled. Run with: ./ircserv <port> <password>\033[0m"

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

# get_client:
# 	@echo "\033[0;32mDownloading and launching KVIrc client...\033[0m"
# 	@curl -o KVIrc-5.0.0.dmg ftp://ftp.kvirc.net/pub/kvirc/5.0.0/binary/macosx/KVIrc-5.0.0.dmg
# 	@open KVIrc-5.0.0.dmg

.PHONY: all clean fclean re


