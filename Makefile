CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I src
LDLIBS  = -lm

all: test_packer

test_packer: src/can_packer.c test/test_packer.c src/can_packer.h
	$(CC) $(CFLAGS) -o $@ src/can_packer.c test/test_packer.c $(LDLIBS)

test: test_packer
	./test_packer

clean:
	rm -f test_packer

.PHONY: all test clean
