# Compiler and flags
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -pthread -lrt

USE_AESD_CHAR_DEVICE ?= 1

ifeq ($(USE_AESD_CHAR_DEVICE),1)
	CFLAGS += -DUSE_AESD_CHAR_DEVICE=1
endif

# Target executable
TARGET ?= aesdsocket

# Source and object files
SRCS = aesdsocket.c \
       aesdsocket_app/src/socket_ops.c \
       aesdsocket_app/src/signal_handler.c \
       aesdsocket_app/src/thread_manager.c
OBJS = $(SRCS:.c=.o)

# Include paths
CFLAGS += -I./aesdsocket_app/include

# Default target builds the executable
.DEFAULT_GOAL := all

all: $(TARGET)

# Build the executable cc -Wall -Wextra -g -O2 -o aesdsocket aesdsocket.o
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files to object files
# cc -Wall -Wextra -g -O2 -c aesdsocket.c -o aesdsocket.o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
