1. 先在root下git clone https://github.com/RedPitaya/RedPitaya.git
2. cd RedPitaya
3. make api，新build出來的librp.so and librp.a會在RedPitaya/build/lib裡
4. 在/opt/新增 quantaser/lib/，將剛剛的librp*放入
5. 修改~/Quantaser_RP之make file:
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
6. run程式時前面改用: LD_LIBRARY_PATH=/opt/quantaser/lib ./


