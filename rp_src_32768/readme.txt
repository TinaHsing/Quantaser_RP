1. ���broot�Ugit clone https://github.com/RedPitaya/RedPitaya.git
	     git clone https://github.com/adam1025/Quantaser_RP.git
2. cd RedPitaya
3.  ��rp_src_32768�̭���generate.c generate.h �ƻs��/root/RedPitaya/api/src��
4. mark api/include/redpitaya/rp.h�̩w�q��BUFFER_LENGTH (�e���[�W"//")
4. make api�A�sbuild�X�Ӫ�librp.so and librp.a�|�bRedPitaya/api/lib�� 
5. �b/opt/�s�W quantaser/lib/ ��Ƨ��A�N��誺librp*��J
6. �ק�~/Quantaser_RP��make file:
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


