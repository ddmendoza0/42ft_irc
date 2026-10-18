NAME		= ircserv

CXX			= c++
CXXFLAGS 	= -Wall -Wextra -Werror -std=c++98 -I includes -MMD -MP

SRC_DIR  = srcs
OBJ_DIR  = objs

SRCS		= $(SRC_DIR)/main.cpp \
				$(SRC_DIR)/Server.cpp \
				$(SRC_DIR)/Client.cpp \
				$(SRC_DIR)/Channel.cpp

OBJS     = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS     = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "Built $(NAME) successfully"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@echo "Cleaned object files"

fclean: clean
	@rm -f $(NAME)
	@echo "Cleaned $(NAME)"

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re