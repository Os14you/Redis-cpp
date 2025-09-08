# Compiler and flags
CXX = g++
# -Iinclude tells the compiler to look for header files in the 'include' directory
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Find all .cpp files in the source directory
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Define object files for the server and client based on their source files
# This creates a list of .o files that will be placed in the BUILD_DIR
SERVER_OBJS = $(BUILD_DIR)/server-main.o $(BUILD_DIR)/Server.o $(BUILD_DIR)/Network.o $(BUILD_DIR)/Redis.o $(BUILD_DIR)/HashTable.o $(BUILD_DIR)/Serialization.o
CLIENT_OBJS = $(BUILD_DIR)/redis-cli.o $(BUILD_DIR)/Client.o $(BUILD_DIR)/Network.o $(BUILD_DIR)/Deserialization.o

# Executable names
SERVER_TARGET = $(BIN_DIR)/redis-server
CLIENT_TARGET = $(BIN_DIR)/redis-cli

# --- Targets ---

# Default target: build all executables
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Rule to link the server executable
$(SERVER_TARGET): $(SERVER_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS)

# Rule to link the client executable
$(CLIENT_TARGET): $(CLIENT_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS)

# Generic rule to compile any .cpp file from src/ into an object file in build/
# This is the core of the build process. It creates the .o files.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rules to create the output directories if they don't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Phony target for cleaning up all build artifacts
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)

.PHONY: all clean