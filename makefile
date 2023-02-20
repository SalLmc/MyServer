PROGS = libmy.so main signal_test log_test timer_test memory_test 

SRCDIRS = src/buffer src/core src/event src/http src/log src/timer src/util src

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

main: main.o
	$(BUILDEXEWITHLIB)

signal_test: signal_test.o
	$(BUILDEXEWITHLIB)

log_test: log_test.o
	$(BUILDEXEWITHLIB)

timer_test: timer_test.o
	$(BUILDEXEWITHLIB)

memory_test: memory_test.o
	$(BUILDEXEWITHLIB)

test: test.o
	$(BUILDEXEWITHLIB)

clean:
	rm -f $(PROGS) $(CPP_OBJECTS) *.o log/* pid_file

re:
	rm -f ./*.o log/* && make

%.o: %.cpp
	g++ $(FLAGS) -fPIC -c $< -o $@