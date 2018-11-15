#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h> 
#include <termios.h> 
#include <errno.h>
#include <math.h>

#include "redpitaya/rp.h"

#define NOUT3 978 
#define NOUT4 979
#define NOUT5 980
#define NOUT6 981
#define NOUT7 982
#define NOUT8 983

int main(void)
{
	pin_export(NOUT3);
	pin_export(NOUT4);
	pin_export(NOUT5);
	pin_export(NOUT6);
	pin_export(NOUT7);
	pin_export(NOUT8);
	pin_direction(NOUT3, OUT);
	pin_direction(NOUT4, OUT);
	pin_direction(NOUT5, OUT);
	pin_direction(NOUT6, OUT);
	pin_direction(NOUT7, OUT);
	pin_direction(NOUT8, OUT);
	pin_write( NOUT3, 0);
	pin_write( NOUT4, 1);
	pin_write( NOUT5, 0);
	pin_write( NOUT6, 1);
	pin_write( NOUT7, 0);
	pin_write( NOUT8, 1);
	printf("Push enter key to start scan :\n");
	getchar();
	pin_unexport(NOUT3);
	pin_unexport(NOUT4);
	pin_unexport(NOUT5);
	pin_unexport(NOUT6);
	pin_unexport(NOUT7);
	pin_unexport(NOUT8);
}