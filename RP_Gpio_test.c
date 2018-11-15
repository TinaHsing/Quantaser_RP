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

#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1

static int pin_export(int);
static int pin_unexport(int);
static int pin_direction(int, int);
static int pin_write(int, int);

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
	pin_write( NOUT3, 1);
	pin_write( NOUT4, 0);
	pin_write( NOUT5, 1);
	pin_write( NOUT6, 0);
	pin_write( NOUT7, 1);
	pin_write( NOUT8, 0);
	getchar();
	pin_write( NOUT3, 0);
	pin_write( NOUT4, 0);
	pin_write( NOUT5, 0);
	pin_write( NOUT6, 0);
	pin_write( NOUT7, 0);
	pin_write( NOUT8, 0);
	pin_unexport(NOUT3);
	pin_unexport(NOUT4);
	pin_unexport(NOUT5);
	pin_unexport(NOUT6);
	pin_unexport(NOUT7);
	pin_unexport(NOUT8);
}

static int pin_export(int pin)
{
	char shell[MAX_PATH];
	sprintf(shell,"echo %d > /sys/class/gpio/export", pin);
	system(shell);
	return 0;
}

static int pin_unexport(int pin)
{
        char shell[MAX_PATH];
        sprintf(shell,"echo %d > /sys/class/gpio/unexport", pin);
        system(shell);

	return 0;
}

static int pin_direction(int pin, int dir){

	char shell[MAX_PATH];
	snprintf(shell, MAX_PATH, "echo %s > /sys/class/gpio/gpio%d/direction",((dir==IN)?"in":"out"),pin);
	system(shell);

	return 0;
}

static int pin_write(int pin, int value)
{
	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	// get pin value file descrptor
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Unable to to open sysfs pins value file %s for writing\n",path);
		return -1;
	}
	if(value==LOW){
		//write low
		if (1 != write(fd, "0", 1)) {
			fprintf(stderr, "Unable to write value\n");
			return -1;
		}
	}
        else if(value==HIGH){
		//write high
		if (1 != write(fd, "1", 1)) {
                	fprintf(stderr, "Unable to write value\n");
                	return -1;
		}
	}else fprintf(stderr, "Nonvalid pin value requested\n");

	//close file
	close(fd);
	return 0;
}