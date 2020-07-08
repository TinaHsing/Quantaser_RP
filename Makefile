CFLAGS  = -g -std=gnu99 -Wall -Werror
CFLAGS += -I/opt/redpitaya/include
LDFLAGS = -L/opt/quantaser/lib
LDLIBS = -lm -lpthread -lrp

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=)

all: $(OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) *.o
	$(RM) $(OBJS)
