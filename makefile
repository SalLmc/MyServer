PROGS = $(BUILDPATH)/libmy.so $(BUILDPATH)/main \
		$(BUILDPATH)/signal_test $(BUILDPATH)/log_test $(BUILDPATH)/timer_test $(BUILDPATH)/memory_test $(BUILDPATH)/test
		
PRE_HEADER = src/headers.h.gch

SRCDIRS = src/buffer src/core src/event src/http src/log src/memory src/timer src/util src

CPP_SOURCES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJECTS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))
INCLUDE_FILES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.d)) *.d

ARSTATICLIB = ar rcs $@ $^
CPPSHARELIB = g++ -fPIC -shared $^ -o $@

LINK = -pthread
FLAGS = -Wall -fPIC -g -MD

BUILDEXEWITHLIB = g++ $(FLAGS) $^ $(BUILDPATH)/libmy.so -o $@ $(LINK)
BUILDEXE = g++ $(FLAGS) $^ -o $@ $(LINK)

BUILDPATH = build

all: build $(PRE_HEADER) $(PROGS)

build:
	if [ ! -d "build" ]; then \
		mkdir build; \
	fi

src/headers.h.gch: src/headers.h
	g++ $(FLAGS) src/headers.h

$(BUILDPATH)/libmy.so: $(CPP_OBJECTS)
	$(CPPSHARELIB)

$(BUILDPATH)/main: main.o
	$(BUILDEXEWITHLIB)

$(BUILDPATH)/signal_test: signal_test.o
	$(BUILDEXEWITHLIB)

$(BUILDPATH)/log_test: log_test.o
	$(BUILDEXEWITHLIB)

$(BUILDPATH)/timer_test: timer_test.o
	$(BUILDEXEWITHLIB)

$(BUILDPATH)/memory_test: memory_test.o
	$(BUILDEXEWITHLIB)

$(BUILDPATH)/test: test.o
	$(BUILDEXEWITHLIB)

-include $(INCLUDE_FILES)

clean:
	rm -f $(PROGS) $(CPP_OBJECTS) *.o log/* pid_file $(INCLUDE_FILES)

cleanall:
	rm -f $(PROGS) $(CPP_OBJECTS) *.o log/* pid_file src/headers.h.gch $(INCLUDE_FILES)

cleanlog:
	rm -f log/*

%.o: %.cpp
	g++ $(FLAGS) -c $< -o $@