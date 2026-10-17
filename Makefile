CC = cc
CFLAGS = -Wall -Wextra -std=c99 -O2

OBJS = tile.o vm.o api.o main.o
TARGET = flux9

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

tile.o: tile.c tile.h flux9.h
vm.o: vm.c vm.h tile.h flux9.h
api.o: api.c api.h vm.h flux9.h
main.o: main.c prog.h vm.h flux9.h

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
