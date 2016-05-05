CC=gcc
LIBS=-lpthread
OUTPUT=sensor
CFLAGS=-Wall
FILES=main.c ../protocol.c

all: ../protocol.h
	$(CC) $(CFLAGS) $(FILES) -o $(OUTPUT) $(LIBS)
