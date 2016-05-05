CC=gcc
CFLAGS=-Wall

packing-test: packing-test.c ../src/protocol.c
	$(CC) $(CFLAGS) packing-test.c ../src/protocol.c -o packing-test -I ../src

all: packing-test
