CC=gcc
CFLAGS=-Wall -Wextra #-Wpedantic -Werror

all: lscat

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

lscat: logging.o utils.o lscat.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf *.o lscat

.PHONY: all clean
