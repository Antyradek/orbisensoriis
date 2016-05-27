CC=gcc
LIBS=-lpthread
OUTPUT=sensor
CFLAGS=-Wall
FILES=main.c ../protocol.c

all: ../protocol.h helper.o
	$(CC) $(CFLAGS) $(FILES) helper.o -o $(OUTPUT) $(LIBS)

helper.o: ../helper.c ../helper.h
	$(CC) $(CFLAGS) ../helper.c -c
