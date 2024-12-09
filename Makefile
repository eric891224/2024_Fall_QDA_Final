CXX = g++
CXXFLAGS = -O2 -Wall -fopenmp

SRCS := $(wildcard *.cpp)

# Object files
OBJS = $(SRCS:.cpp=.o)

TARGET = hw3

all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Rule to compile the source files
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)