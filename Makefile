CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lm

SRC = src/main.c \
      src/core/memory.c \
      src/core/arm64.c \
      src/core/kernel.c \
      src/core/svc.c \
      src/core/loader.c \
      src/hle/service.c \
      src/gpu/gpu.c \
      src/gpu/display.c \
      src/audio/audio.c \
      src/input/hid.c \
      src/fs/fs.c

OBJ = $(SRC:.c=.o)
BIN = pumice

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
