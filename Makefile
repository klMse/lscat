CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror -Wshadow
LDFLAGS=-Wl,-z,relro,-z,now

all: lscat

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

lscat: logging.o lscat.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

debug: logging.o lscat.o
	$(CC) $(CFLAGS) -ggdb3 $(LDFLAGS) -o lscat-$@ $^

clean:
	rm -rf *.o lscat lscat-debug

.PHONY: all clean
