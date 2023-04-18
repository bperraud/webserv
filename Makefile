NAME = webserv

SRC = $(wildcard src/*.cpp)

CXX = c++

#CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -fsanitize=address -g

OBJ = $(SRC:.cpp=.o)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

all: $(NAME)

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

run: $(NAME)
	./$(NAME)
