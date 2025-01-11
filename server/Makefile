# Compiler and flags
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -g -O2

# Target executable
TARGET = aesdsocket

# Source and object files
SRCS = aesdsocket.c
OBJS = $(SRCS:.c=.o)

# Default target builds the executable
.DEFAULT_GOAL := all

all: $(TARGET)

# Build the executable cc -Wall -Wextra -g -O2 -o aesdsocket aesdsocket.o
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
# cc -Wall -Wextra -g -O2 -c aesdsocket.c -o aesdsocket.o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
