all: server

server: server.c
	gcc -g -Wall $< -o $@ `pkg-config libwebsockets --libs --cflags`

client: client.c
	gcc -g -Wall $< -o $@ `pkg-config libwebsockets --libs --cflags` -lpthread -lreadline

run: all
	./server
	
runc: client
	./client

clean:
	rm -f server
