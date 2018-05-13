# simple-libwebsockets-game

A simple libwebsockets game example with a chat.
Its very very simplistic, and is based upon http://ahoj.io/libwebsockets-simple-websocket-server.

## Build
```bash
make
```

## Run
In one terminal run:
```bash
./server
```

Or just compile and run with:
```bash
make run
```

Then navigate to localhost:8000 in your web browser. You should be able to write in a chat (Cannot handle too much load), and you should be able to guess and set a box.

There is 3 sending types:

* 1 byte {C} (Chat) + 10 bytes (USER) + N Bytes (Message)
* 1 byte {S} (Set box) + 1 byte (BOX NUM - Probably ASCII 49 - 51 ('1' - '3'))
* 1 byte {G} (Guess box) + 1 byte (BOX NUM - Probably ASCII 49 - 51 ('1' - '3'))

## Things you can improve

* Chat history
* Better conflict handling if more players (can be a mess)
