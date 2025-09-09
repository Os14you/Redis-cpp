# Compiler and flags
CXX = g++
# -Iinclude tells the compiler to look in 'include' for the 'redis_cpp' directory
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude

# Directories
BUILD_DIR = build
BIN_DIR = bin

# --- Source Files ---
# List all .cpp source files from their new locations
SRCS := \
    src/redis_server.cpp \
    src/server/Redis.cpp \
    src/net/Server.cpp \
    src/net/Network.cpp \
    src/core/HashTable.cpp \
    src/common/Serialization.cpp \
    \
    src/redis_cli.cpp \
    src/net/Client.cpp \
    src/common/Deserialization.cpp

# --- Object Files ---
# Generate a list of .o object files that will be placed in the BUILD_DIR.
# This uses text substitution to replace 'src/' with 'build/' and '.cpp' with '.o'
OBJS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Define the object files required for each specific executable
SERVER_OBJS = $(BUILD_DIR)/server-main.o $(BUILD_DIR)/server/Redis.o $(BUILD_DIR)/net/Server.o $(BUILD_DIR)/net/Network.o $(BUILD_DIR)/core/HashTable.o $(BUILD_DIR)/common/Serialization.o
CLIENT_OBJS = $(BUILD_DIR)/redis-cli.o $(BUILD_DIR)/net/Client.o $(BUILD_DIR)/net/Network.o $(BUILD_DIR)/common/Deserialization.o

# Executable names
SERVER_TARGET = $(BIN_DIR)/redis-server
CLIENT_TARGET = $(BIN_DIR)/redis-cli

# --- Targets ---

# Default target: build all executables
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Rule to link the server executable
$(SERVER_TARGET): $(SERVER_OBJS)
	@mkdir -p $(@D) # Ensure the bin/ directory exists
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to link the client executable
$(CLIENT_TARGET): $(CLIENT_OBJS)
	@mkdir -p $(@D) # Ensure the bin/ directory exists
	$(CXX) $(CXXFLAGS) -o $@ $^

# This is the core compilation rule. It matches any .o file in the build directory
# and finds its corresponding .cpp file in the src directory.
$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(@D) # Create the subdirectory in build/ (e.g., build/net, build/core)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Phony target for cleaning up all build artifacts
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

# .PHONY tells make that 'all' and 'clean' are not actual files
.PHONY: all clean