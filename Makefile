# Chip8 Emulator in C
# Author: Adrian Stein <adrian.stein@tum.de>
# Version: 0.1.1
# License: MIT
# Description: Chip8 emulator

CC = gcc
CFLAGS = -I src/include/SDL2
LDFLAGS = -L src/lib -lmingw32 -lSDL2main -lSDL2

SRCS = main.c memory.c cpu.c vmemory.c timer.c random_byte.c display.c input.c sound.c chip8.c
OBJS = $(SRCS:.c=.o)
TARGET = chip8emu_c

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
