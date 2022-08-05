# autor: Ismail Serbest
# matrikelnummer: 01129764

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c

.PHONY: all clean

all: generator.o supervisor.o
	$(CC) -o generator generator.o -lrt -pthread
	$(CC) -o supervisor supervisor.o -lrt -pthread

generator.o: generator.c
	$(CC) $(CFLAGS) generator.c

supervisor.o: supervisor.c
	$(CC) $(CFLAGS) supervisor.c

clean:
	-rm -rf supervisor supervisor.o generator generator.o