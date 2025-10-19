NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRC_DIR = srcs/
OBJ_DIR = objs/
INC_DIR = inc/

SRCS = main.cpp \
       Server.cpp \
       Client.cpp \
       Channel.cpp \
	   Reply.cpp \
	   IRCCommand.cpp \
	   ChannelsClientsManager.cpp

OBJS = $(addprefix $(OBJ_DIR), $(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJ_DIR) $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "Compilation complete. Executable: $(NAME)"

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	@echo "Object files removed"

fclean: clean
	rm -f $(NAME)
	@echo "Executable removed"

set_google_test:
	git submodule update --init --recursive

test_clean:
	rm -rf googletests

re: fclean all

run_server: re
	./ft_irc_serv 1201 123

gdb: re
	gdb ./ft_irc_serv

debug_run:
	make re CXXFLAGS="-g -O0 -std=c++98 -Wall -Wextra"
	gdb --args ./ft_irc_serv 1201 123

valgrind_deep: re
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes ./ft_irc_serv 1201 123

clone_google_test:
	mkdir -p googletests/googletest
	git clone https://github.com/google/googletest.git googletests/googletest

gdb: re
	gdb ./ft_irc_serv

debug_run:
	make re CXXFLAGS="-g -O0 -std=c++98 -Wall -Wextra"
	gdb --args ./ft_irc_serv 1201 123

set_build:
	mkdir -p googletests/build
	cd googletests/build && cmake ..
	cd googletests/build && make
	@echo "Tests should be compiled in the build directory"

.PHONY: all clean fclean re

# in build directory
# make irc_commands
# make cha...
# make irc_commands && ./command_test
