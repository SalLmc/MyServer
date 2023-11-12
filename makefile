PROGS = libmy.so main 

PRE_HEADER = src/headers.h.gch

SRCDIRS = src/buffer src/core src/event src/http src/log src/memory src/timer src/utils \
          src ./

CPP_SOURCES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJECTS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))
INCLUDE_FILES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.d)) *.d

CPPSHARELIB = $(CXX) -fPIC -shared $^ -o $@

LINK = -pthread
CXXFLAGS = -Wall -fPIC -g -MD
CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")

BUILDEXEWITHLIB = $(CXX) $(CXXFLAGS) $^ ./libmy.so -o $@ $(LINK)
BUILDEXE = $(CXX) $(CXXFLAGS) $^ -o $@ $(LINK)

CONFIG = config.json

all: $(CONFIG) $(PRE_HEADER) $(PROGS) tests

src/headers.h.gch: src/headers.h
	$(CXX) $(CXXFLAGS) src/headers.h

libmy.so: $(CPP_OBJECTS)
	$(CPPSHARELIB)

main: main.o $(CPP_OBJECTS)
	$(BUILDEXE)

# main: main.o
# 	$(BUILDEXEWITHLIB)

tests:
	$(MAKE) -C test

-include $(INCLUDE_FILES)

clean:
	rm -f $(PROGS) $(CPP_OBJECTS) $(INCLUDE_FILES)
	$(MAKE) clean -C test
