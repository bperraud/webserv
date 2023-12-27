NAME = webserv

SRC = $(wildcard src/*.cpp)

#CXX = c++
CXX = g++-13

CXXFLAGS = -Iinclude -O3
LDFLAGS = -lssl -lcrypto  # OpenSSL flags for linking
#CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -O3
OBJ = $(SRC:.cpp=.o)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)

all: $(NAME)

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

run: $(NAME)
	./$(NAME)
