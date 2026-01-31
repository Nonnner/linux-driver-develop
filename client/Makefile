# Makefile for Chat Client
#
# This Makefile builds the chat client application

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
INCLUDES = -I../common
LDFLAGS = -pthread

# Source files
SRC = chat_client.c
OBJ = $(SRC:.c=.o)
TARGET = chat_client

# Default target
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Clean target
clean:
	rm -f $(OBJ) $(TARGET)

# Run target
run: $(TARGET)
	./$(TARGET)

# Debug target
debug: $(TARGET)
	gdb ./$(TARGET)

.PHONY: all clean run debug
