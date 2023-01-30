PROGS = ./build/libmy.so echosv test

OBJS = ./src/buffer/buffer.o ./src/core/core.o ./src/event/epoller.o ./src/log/nanolog.o

ARSTATICLIB = ar rcs $@ $^
CPPSHARELIB = g++ -fPIC -shared $^ -o $@

LINK = -pthread
FLAGS = -Wall -g
BUILDEXE = g++ $(FLAGS) $^ ./build/libmy.so -o $@ $(LINK)

all: $(PROGS)

./build/libmy.so: $(OBJS)
	$(CPPSHARELIB)

echosv: ./src/http/echosv.o
	$(BUILDEXE)

test: test.o
	$(BUILDEXE)

clean:
	rm -f $(PROGS) $(OBJS) ./src/http/echosv.o test.o

.cpp.o:
	g++ $(FLAGS) -fPIC -c $< -o $@