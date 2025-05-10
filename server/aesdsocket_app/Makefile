CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
INCLUDES = -I./include

SRC_DIR = src
SRCS = $(SRC_DIR)/socket_ops.c \
       $(SRC_DIR)/thread_manager.c \
       $(SRC_DIR)/signal_handler.c

OBJS = $(SRCS:.c=.o)
LIB = libaesdsocket.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(LIB) $(OBJS)
	rm -f $(SRC_DIR)/*.o
