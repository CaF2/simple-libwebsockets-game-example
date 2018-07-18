all: server

server: server.c sending.c
	gcc -g -Wall $^ -o $@ `pkg-config --libs --cflags libwebsockets glib-2.0` -D_GNU_SOURCE -g

run: all
	./server
	
gdb: all
	gdb --args ./server
	
valgrind: all
	valgrind  --track-origins=yes --tool=memcheck --leak-check=full ./server

clean:
	rm -f server
