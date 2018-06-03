all: server

server: server.c
	gcc -g -Wall $< -o $@ `pkg-config --libs --cflags libwebsockets`

client: client.c
	gcc -g -Wall $< -o $@ `pkg-config --libs --cflags libwebsockets` -lpthread -lreadline

run: all
	./server
	
runc: client
	./client

clean:
	rm -f server
