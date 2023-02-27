PROGS = libcolib.a libcolib.so test1

COLIB_OBJS = co_cond.o co_epoll.o co_timer.o co.o coctx_swap.o coctx.o

ARSTATICLIB = ar rcs $@ $^
CPPSHARELIB = g++ -fPIC -shared $^ -o $@

BUILDEXE = g++ -Wall $^ ./libcolib.so -o $@
BUILDWITHOUTLIB = g++ -Wall $^ -o $@

all: $(PROGS)

libcolib.a: $(COLIB_OBJS)
	$(ARSTATICLIB)

libcolib.so: $(COLIB_OBJS)
	$(CPPSHARELIB)

# test: test.o
# 	$(BUILDEXE)

test1: test1.o $(COLIB_OBJS)
	$(BUILDWITHOUTLIB)

retest1:
	rm -f test1.o && make

clean:
	rm -f *.o $(PROGS)

.cpp.o:
	g++ -Wall -fPIC -c $< -o $@
