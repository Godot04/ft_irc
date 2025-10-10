NAME = ft_irc_serv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRC_DIR = srcs/
OBJ_DIR = objs/
INC_DIR = inc/

SRCS = main.cpp \
       Server.cpp \
       Client.cpp \
       Channel.cpp \
       Command.cpp \
	   Reply.cpp \
	   Server_Commands.cpp \

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

.PHONY: all clean fclean re
