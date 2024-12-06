CXX = g++
CXXFLAG = -O2 -Wall

SRCS := $(wildcard *.cpp)

# Object files
OBJS = $(SRCS:.cpp=.o)

TARGET = hw3

all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -fopenmp -o $(TARGET)

# Rule to compile the source files
.cpp.o:
	$(CXX) $(CXXFLAGS) -fopenmp -c $< -o $@
	
# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)