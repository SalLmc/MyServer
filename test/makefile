SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, %.o, $(SRCS))
EXES = $(patsubst %.cpp, %, $(SRCS))

INCLUDE_FILES = $(wildcard *.d)

LINK = -pthread
CXXFLAGS = -Wall -fPIC -g -MD
# CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
CXX = g++

BUILDEXEWITHLIB = $(CXX) $(CXXFLAGS) $^ ../libmy.so -o $@ $(LINK)

all: $(EXES)
	
$(EXES): %: %.o
	$(BUILDEXEWITHLIB)

-include $(INCLUDE_FILES)

clean:
	rm -f $(OBJS) $(EXES) $(INCLUDE_FILES)
