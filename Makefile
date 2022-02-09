CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror
LDFLAGS=-Wl,-z,relro,-z,now

all: lscat

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

lscat: logging.o lscat.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -rf *.o lscat

.PHONY: all clean
