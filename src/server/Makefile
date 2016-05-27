CC=gcc
LIBS=-lpthread
OUTPUT=server
CFLAGS=-Wall
FILES=main.c ../protocol.c

all: ../protocol.h helper.o
	$(CC) $(CFLAGS) $(FILES) helper.o -o $(OUTPUT) $(LIBS)
helper.o: ../helper.h ../helper.c
	$(CC) $(CFLAGS) ../helper.c -c
