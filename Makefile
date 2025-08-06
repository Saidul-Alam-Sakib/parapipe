CC=gcc
CFLAGS=-Wall -pthread

all: parapipe

parapipe: parapipe.c
	$(CC) $(CFLAGS) -o parapipe parapipe.c
