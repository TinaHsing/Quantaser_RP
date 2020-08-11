1. 先在root下git clone https://github.com/RedPitaya/RedPitaya.git
2. cd RedPitaya
3.  把rp_src_32768裡面的generate.c geberate.h 複製到/root/RedPitaya/api/src裡
4. make api，新build出來的librp.so and librp.a會在RedPitaya/api/lib裡 (api/include/redpitaya/rp.h裡會重複定義BUFFER_LENGTH，需mark //)
5. 在/opt/新增 quantaser/lib/資料夾，將剛剛的librp*放入
6. 修改~/Quantaser_RP之make file:   (@root/ git clone https://github.com/adam1025/Quantaser_RP.git)
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
7. run程式時前面改用: LD_LIBRARY_PATH=/opt/quantaser/lib ./


