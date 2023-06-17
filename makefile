CC = gcc
CFLAGS = -Wall -Wextra

all: server client

server: server.o common.o
	$(CC) $(CFLAGS) $^ -o $@

client: client.o common.o
	$(CC) $(CFLAGS) $^ -o $@

sconnect:
	./server 127.0.0.1 64000 60001

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o server client
