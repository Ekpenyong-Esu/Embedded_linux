# Compiler and flags
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -g

# Target application name
TARGET = writer

# Source and object files
SRCS = writer.c
OBJS = $(SRCS:.c=.o)

# Default target: Build the application
all: $(TARGET)

# Build the application
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target: Remove generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
