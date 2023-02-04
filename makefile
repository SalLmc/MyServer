PROGS = libmy.so signal_test log_test echosv main

SRCDIRS = src/buffer src/core src/event src/http src/log src/util src

CPP_SOURCES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJECTS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))

ARSTATICLIB = ar rcs $@ $^
CPPSHARELIB = g++ -fPIC -shared $^ -o $@

LINK = -pthread
FLAGS = -Wall -g

BUILDEXEWITHLIB = g++ $(FLAGS) $^ ./libmy.so -o $@ $(LINK)
BUILDEXE = g++ $(FLAGS) $^ -o $@ $(LINK)

all: $(PROGS)

libmy.so: $(CPP_OBJECTS)
	$(CPPSHARELIB)

signal_test: signal_test.o
	$(BUILDEXEWITHLIB)

log_test: log_test.o
	$(BUILDEXEWITHLIB)

echosv: echosv.o
	$(BUILDEXEWITHLIB)

main: main.o
	$(BUILDEXEWITHLIB)

clean:
	rm -f $(PROGS) $(CPP_OBJECTS) *.o

%.o: %.cpp
	g++ $(FLAGS) -fPIC -c $< -o $@