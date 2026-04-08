# IRC Server Makefile
# C++98 compliant, works on Linux and Mac

NAME = ircserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

# Detect OS and set platform-specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
# macOS
CXXFLAGS += -D__MACOS__
endif
ifeq ($(UNAME_S),Linux)
# Linux
CXXFLAGS += -D__LINUX__
endif

# Directories
SRCDIR = src
INCDIR = include/irc
OBJDIR = objs

# Source files (to be added as development progresses)
SOURCES = $(wildcard $(SRCDIR)/*.cpp) \
$(wildcard $(SRCDIR)/commands/*.cpp)

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Header files for dependency tracking
INCLUDES = $(wildcard $(INCDIR)/*.hpp) \
$(wildcard $(INCDIR)/commands/*.hpp)

# Default target
all: $(NAME)

# Create object directory structure
$(OBJDIR):
	@mkdir -p $(OBJDIR)
	@mkdir -p $(OBJDIR)/commands

# Link executable (only if object files exist)
$(NAME): $(OBJDIR) $(OBJECTS)
	@if [ -z "$(OBJECTS)" ]; then \
		echo "No source files found. Please add source files to $(SRCDIR)/"; \
		exit 1; \
	fi
	@echo "Linking $(NAME)..."
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(NAME)
	@echo "$(NAME) compiled successfully"

# Compile source files to object files
# Dependencies: only recompile if source or headers changed
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

# Clean object files
clean:
	@echo "Cleaning object files..."
	@rm -rf $(OBJDIR)
	@echo "Clean complete"

# Full clean (objects + executable + bot binary)
fclean: clean
	@echo "Removing $(NAME)..."
	@rm -f $(NAME)
	@rm -f $(BOT_NAME)
	@rm -f $(PLAYBOT_NAME)
	@echo "Full clean complete"

# Rebuild (fclean + all)
re: fclean all

# ── bonus: IRC bot (channel flood tester) ────────────────────────────────────
# Compiled separately; does NOT affect the main server build.
# bot binary is standalone — uses only standard C++98 + POSIX sockets.
BOT_NAME = channel_flood_bot
BOT_SRC  = src/bot/channel_flood_bot.cpp

bot: $(BOT_SRC)
	@echo "Compiling $(BOT_NAME)..."
	$(CXX) $(CXXFLAGS) -o $(BOT_NAME) $(BOT_SRC)
	@echo "$(BOT_NAME) compiled successfully"

# ── bonus: PlayBot (command dispatch + reconnect) ─────────────────────────────
# BotCore (transport) + BotCommands (dispatch) + BotMain (loop)
PLAYBOT_NAME = playbot
PLAYBOT_SRC  = src/bot/BotCore.cpp src/bot/BotCommands.cpp src/bot/BotMain.cpp
PLAYBOT_OBJ  = $(PLAYBOT_SRC:%.cpp=%.o)

playbot: $(PLAYBOT_SRC)
	@echo "Compiling $(PLAYBOT_NAME)..."
	$(CXX) $(CXXFLAGS) -Iinclude -o $(PLAYBOT_NAME) $(PLAYBOT_SRC)
	@echo "$(PLAYBOT_NAME) compiled successfully"

# Phony targets
.PHONY: all clean fclean re bot playbot
