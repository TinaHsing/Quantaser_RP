1. ���broot�Ugit clone https://github.com/RedPitaya/RedPitaya.git
2. cd RedPitaya
3. make api�A�sbuild�X�Ӫ�librp.so and librp.a�|�bRedPitaya/build/lib��
4. �b/opt/�s�W quantaser/lib/�A�N��誺librp*��J
5. �ק�~/Quantaser_RP��make file:
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
6. run�{���ɫe�����: LD_LIBRARY_PATH=/opt/quantaser/lib ./


