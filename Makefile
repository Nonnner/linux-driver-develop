# Makefile for chat server and client
CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
LDFLAGS = -pthread

.PHONY: all clean

all: chat_server chat_client

chat_server: chat_server.c
	$(CC) $(CFLAGS) -o chat_server chat_server.c $(LDFLAGS)

chat_client: chat_client.c
	$(CC) $(CFLAGS) -o chat_client chat_client.c $(LDFLAGS)

clean:
	rm -f chat_server chat_client
