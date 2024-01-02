PROGS = libmy.so main 

SRCDIR = src 

PRE_HEADER = src/headers.h.gch

CPP_SOURCES = $(shell find $(SRCDIR) -name '*.cpp')
CPP_OBJECTS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))

INCLUDE_FILES = $(shell find ./ -name '*.d')

CPPSHARELIB = $(CXX) -fPIC -shared $^ -o $@

LINK = -pthread
CXXFLAGS = -Wall -fPIC -g -MD
# CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
CXX = g++

BUILDEXEWITHLIB = $(CXX) $(CXXFLAGS) $^ ./libmy.so -o $@ $(LINK)
BUILDEXE = $(CXX) $(CXXFLAGS) $^ -o $@ $(LINK)

CONFIG = config.json types.json

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
	rm -f $(PROGS) $(CPP_OBJECTS) $(INCLUDE_FILES) main.o
	$(MAKE) clean -C test
