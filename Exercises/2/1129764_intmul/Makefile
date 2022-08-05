.PHONY: all clean

all: intmul

intmul: intmul.o
	gcc -o intmul intmul.o -lm

intmul.o: intmul.c
	gcc -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SCID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c intmul.c

clean:
	rm -f intmul *.o