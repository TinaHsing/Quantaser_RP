//////*header include*//////
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

/* MOS SW*/
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
#define POUT1 968 //DIO0_P
#define POUT2 969
#define POUT3 970
#define POUT4 971
#define UART1 972
#define UART2 973
#define UART3 974
#define UART4 975 //DIO7_P

////////* gpio *///////////
static int pin_export(int);
static int pin_unexport(int);
static int pin_direction(int, int);
static int pin_write(int, int);

/******MOS Switch******/
int mos_sw1, mos_sw2, mos_sw3, mos_sw4;

int main(int argc, char *argv[])
{
	// printf("--Selecting Function MOS Switch---\n");
	// printf("set switch status : on(1), off(0)\n");
	// printf("SW1 SW2 SW3 SW4 (ex: 1 0 0 1)");
	// scanf("%d%d%d%d", &mos_sw1, &mos_sw2, &mos_sw3, &mos_sw4);
	mos_sw1 = atoi(argv[1]);
	mos_sw2 = atoi(argv[2]);
	mos_sw3 = atoi(argv[3]);
	mos_sw4 = atoi(argv[4]);
	pin_export(POUT1);
	pin_export(POUT2);
	pin_export(POUT3);
	pin_export(POUT4);
	pin_direction(POUT1, OUT);
	pin_direction(POUT2, OUT);
	pin_direction(POUT3, OUT);
	pin_direction(POUT4, OUT);
	// pin_write( POUT1, 0);
	// pin_write( POUT2, 0);
	// pin_write( POUT3, 0);
	// pin_write( POUT4, 0);
	if(mos_sw1) pin_write( POUT1, 1);
	else pin_write( POUT1, 0);
	if(mos_sw2) pin_write( POUT2, 1);
	else pin_write( POUT2, 0);
	if(mos_sw3) pin_write( POUT3, 1);
	else pin_write( POUT3, 0);
	if(mos_sw4) pin_write( POUT4, 1);
	else pin_write( POUT4, 0);
	pin_unexport(POUT1);
	pin_unexport(POUT2);
	pin_unexport(POUT3);
	pin_unexport(POUT4);
	return 0;
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