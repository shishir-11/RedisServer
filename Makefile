# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -Wextra -pthread -MMD -MP -O2

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Target binary
TARGET = $(BIN_DIR)/server

# Find all .cpp files in the src directory
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default rule
all: $(TARGET)

# Ensure build and bin directories exist
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# Compile .cpp files into .o object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link all object files into the final executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Clean up build and output files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Rebuild from scratch
rebuild: clean all

# Run the executable
run: all
	./$(TARGET)

# CXX = g++
# CXXFLAGS = -Wall -pthread -MMD -MP -02

# SRC_DIR = src
# BUILD_DIR = build

# SRCS := $(wildcard $(SRC_DIR)/*.cpp)
# OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o,$(SRCS))

# TARGET = server

# all: $(Target)

# $(BUILD_DIR):
# 	mkdir -p $(BUILD_DIR)

# $(BUILD_DIR)/%.o:%(SRC_DIR)/%.cpp | 
