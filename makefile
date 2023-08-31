PROGS = libmy.so main \
		signal_test log_test timer_test memory_test test
		
PRE_HEADER = src/headers.h.gch

SRCDIRS = src/buffer src/core src/event src/http src/log src/memory src/timer src/utils src

CPP_SOURCES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJECTS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))
INCLUDE_FILES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.d)) *.d

ARSTATICLIB = ar rcs $@ $^
CPPSHARELIB = g++ -fPIC -shared $^ -o $@

LINK = -pthread
FLAGS = -Wall -fPIC -g -MD

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

-include $(INCLUDE_FILES)

clean:
	rm -f $(PROGS) $(CPP_OBJECTS) $(INCLUDE_FILES) *.o pid_file cores

cleanall:
	rm -f $(PROGS) $(CPP_OBJECTS) $(INCLUDE_FILES) *.o pid_file src/headers.h.gch

%.o: %.cpp
	g++ $(FLAGS) -c $< -o $@