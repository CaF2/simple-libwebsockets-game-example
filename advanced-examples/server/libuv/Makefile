all: server

server: server.c
	gcc -g -Wall $< -o $@ `pkg-config --libs --cflags libwebsockets libuv`

run: all
	./server

clean:
	rm -f server
