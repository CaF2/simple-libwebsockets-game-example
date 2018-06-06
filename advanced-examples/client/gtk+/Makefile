# Install
BIN = client

CC = gcc

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

# Flags
PKG_CONFIG = libwebsockets gtk+-3.0

CFLAGS = -D_GNU_SOURCE -g
CFLAGS += $(if $(PKG_CONFIG),$(shell pkg-config --cflags $(PKG_CONFIG)))

LDFLAGS = -lm -pthread
LDFLAGS += $(if $(PKG_CONFIG),$(shell pkg-config --libs $(PKG_CONFIG)))

all: $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)
	
run: all
	./$(BIN)
