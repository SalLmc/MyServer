SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, %.o, $(SRCS))
EXES = $(patsubst %.cpp, %, $(SRCS))

INCLUDE_FILES = $(wildcard *.d)

LINK = -pthread
FLAGS = -Wall -fPIC -g -MD
CC = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")

BUILDEXEWITHLIB = $(CC) $(FLAGS) $^ ../libmy.so -o $@ $(LINK)

all: $(EXES)
	
log_test: log_test.o
	$(BUILDEXEWITHLIB)

memory_test: memory_test.o
	$(BUILDEXEWITHLIB)

proxy_pass_test: proxy_pass_test.o
	$(BUILDEXEWITHLIB)

signal_test: signal_test.o
	$(BUILDEXEWITHLIB)

timer_test: timer_test.o
	$(BUILDEXEWITHLIB)

test: test.o
	$(BUILDEXEWITHLIB)

-include $(INCLUDE_FILES)

clean:
	rm -f $(OBJS) $(EXES) $(INCLUDE_FILES)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@