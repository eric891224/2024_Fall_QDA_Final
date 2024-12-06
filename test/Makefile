# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -I .. 

# Source and Target
SRC = io_test.cpp
OBJ = io_test.o
BIN = io_test

# Default Target
all: $(BIN)

# Compile Object File
$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link Executable
$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@

# Clean Rule
clean:
	rm -f $(OBJ) $(BIN)

