1. ���broot�Ugit clone https://github.com/RedPitaya/RedPitaya.git
2. cd RedPitaya
3.  ��rp_src_32768�̭���generate.c geberate.h �ƻs��/root/RedPitaya/api/src��
4. make api�A�sbuild�X�Ӫ�librp.so and librp.a�|�bRedPitaya/api/lib�� (api/include/redpitaya/rp.h�̷|���Ʃw�qBUFFER_LENGTH�A��mark //)
5. �b/opt/�s�W quantaser/lib/��Ƨ��A�N��誺librp*��J
6. �ק�~/Quantaser_RP��make file:   (@root/ git clone https://github.com/adam1025/Quantaser_RP.git)
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
7. run�{���ɫe�����: LD_LIBRARY_PATH=/opt/quantaser/lib ./


