CC = gcc
CFLAGS = -I src/include/SDL2
LDFLAGS = -L src/lib -lmingw32 -lSDL2main -lSDL2

SRCS = main.c memory.c cpu.c vmemory.c timer.c display.c input.c sound.c debugger.c chip8.c
OBJS = $(SRCS:.c=.o)
TARGET = chip8

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
