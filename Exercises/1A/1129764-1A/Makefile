# program-name: ispalindrom
# autor: Ismail Serbest
# matrikelnummer: 01129764

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c

all: ispalindrom

ispalindrom: ispalindrom.o
	$(CC) -o ispalindrom ispalindrom.o

$(FILENAME).o: ispalindrom.c
	$(CC) $(CFLAGS) ispalindrom.c

clean:
	rm -f $(FILENAME).o $(FILENAME)
