PROGS = libmy.so main \
		test/signal_test test/log_test test/timer_test test/memory_test test/test
		
PRE_HEADER = src/headers.h.gch

SRCDIRS = src/buffer src/core src/event src/http src/log src/memory src/timer src/utils src

CPP_SOURCES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJECTS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))
INCLUDE_FILES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.d)) *.d

ARSTATICLIB = ar rcs $@ $^
CPPSHARELIB = $(CC) -fPIC -shared $^ -o $@

LINK = -pthread
FLAGS = -Wall -fPIC -g -MD
CC = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")

BUILDEXEWITHLIB = g++ $(FLAGS) $^ ./libmy.so -o $@ $(LINK)
BUILDEXE = g++ $(FLAGS) $^ -o $@ $(LINK)

CONFIG = config.json

all: $(CONFIG) $(PRE_HEADER) $(PROGS)

src/headers.h.gch: src/headers.h
	g++ $(FLAGS) src/headers.h

libmy.so: $(CPP_OBJECTS)
	$(CPPSHARELIB)

main: main.o
	$(BUILDEXEWITHLIB)

test/signal_test: test/signal_test.o
	$(BUILDEXEWITHLIB)

test/log_test: test/log_test.o
	$(BUILDEXEWITHLIB)

test/timer_test: test/timer_test.o
	$(BUILDEXEWITHLIB)

test/memory_test: test/memory_test.o
	$(BUILDEXEWITHLIB)

test/test: test/test.o
	$(BUILDEXEWITHLIB)

-include $(INCLUDE_FILES)

clean:
	rm -f $(PROGS)
	find ./ -name "*.o" -type f -exec echo {} \; -exec rm -f {} \;
	find ./ -name "*.d" -type f -exec echo {} \; -exec rm -f {} \;

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@