# Install
BIN = client

NUKLEAR_PATH = $$HOME/git/nuklear
NUKLEAR_SDL2_GL3_PATH = $(NUKLEAR_PATH)/demo/sdl_opengl3

CC = gcc

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

# Flags
PKG_CONFIG = libwebsockets

CFLAGS = -I$(NUKLEAR_PATH) -D_GNU_SOURCE -g
CFLAGS += $(if $(PKG_CONFIG),$(shell pkg-config --cflags $(PKG_CONFIG)))

LDFLAGS = -lSDL2 -lGL -lm -lGLU -lGLEW -pthread
LDFLAGS += $(if $(PKG_CONFIG),$(shell pkg-config --libs $(PKG_CONFIG)))

all: $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)
	
run: all
	./$(BIN)
