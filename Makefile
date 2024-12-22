# Compiler and Compiler Flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# Target executable
TARGET = sish

# Source files in root
SRCS = sish.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET) test_files_build

# Rule to build the target executable in the root
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to build object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Call
