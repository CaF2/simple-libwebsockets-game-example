all: server

server: server.c
	gcc -g -Wall $< -o $@ `pkg-config libwebsockets --libs --cflags`
	
run: all
	./server

clean:
	rm -f server
