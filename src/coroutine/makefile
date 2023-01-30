PROGS = libcolib.a libcolib.so test test1

COLIB_OBJS = co_cond.o co_epoll.o co_timer.o co.o coctx_swap.o coctx.o

BUILDPATH=./

ARSTATICLIB = ar rcs $(BUILDPATH)$@ $^
CPPSHARELIB = g++ -fPIC -shared $^ -o $(BUILDPATH)$@

BUILDEXE = g++ -Wall $^ $(BUILDPATH)libcolib.so -o $@

all: $(PROGS)

libcolib.a: $(COLIB_OBJS)
	$(ARSTATICLIB)

libcolib.so: $(COLIB_OBJS)
	$(CPPSHARELIB)

test: test.o
	$(BUILDEXE)

test1: test1.o
	$(BUILDEXE)

clean:
	rm -f *.o $(PROGS)

.cpp.o:
	g++ -Wall -fPIC -c $< -o $@
