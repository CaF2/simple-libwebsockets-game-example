# simple-libwebsockets-game

A simple libwebsockets game example with a chat.
Its very very simplistic, and is based upon https://github.com/iamscottmoyers/simple-libwebsockets-example.

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

You can also run the client with (if you like the terminal) Currenly made for linux users :P
```bash
make run
```

There is 3 sending types:

* 1 byte {C} (Chat) + 10 bytes (USER) + N Bytes (Message)
* 1 byte {S} (Set box) + 1 byte (BOX NUM - Probably ASCII 49 - 51 ('1' - '3'))
* 1 byte {G} (Guess box) + 1 byte (BOX NUM - Probably ASCII 49 - 51 ('1' - '3'))

## Things you can improve

* Chat history
* Better conflict handling if more players (can be a mess)
* Nicer frontend
* Better client, GUI?
* Complicate the currently awsome gameplay! Its 10/10, but you can make it better!
* Make it smaller?
