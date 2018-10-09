/* Red Pitaya C API example Generating continuous signal  
 * This application generates a specific signal */

// #include <stdio.h>
// #include <stdint.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/time.h>

// #include "redpitaya/rp.h"
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

#include "redpitaya/rp.h"

#define updateRate 5 //us
#define FGTRIG 983
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
long micros(void);
static int pin_export(int);
static int pin_unexport(int);
static int pin_direction(int, int);
static int pin_write(int, int);
int main(int argc, char **argv){
	float freq;
	long t0, t_now;
	bool flag=0;

	/* Print error, if rp_Init() function failed */
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}
	
	pin_export(FGTRIG);
	pin_direction(FGTRIG, OUT);
	pin_write( FGTRIG, 0);
	/* Generating frequency */
	printf("input freq in Hz: ");
	scanf("%f", &freq);
	// printf("input amp in V: ");
	// scanf("%f", &amp);
	rp_GenFreq(RP_CH_1, freq);
	// rp_GenFreq(RP_CH_2, freq);

	/* Generating amplitude */
	rp_GenAmp(RP_CH_1, 0);
	// rp_GenAmp(RP_CH_2, 1);

	/* Generating wave form */
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
	// rp_GenWaveform(RP_CH_2, RP_WAVEFORM_TRIANGLE);
	/* Enable channel */
	rp_GenOutEnable(RP_CH_1);
	// rp_GenOutEnable(RP_CH_2);
	t0=micros();
	while(1)
	{
		t_now = micros()-t0;
		if(t_now > updateRate)
		{
			if(!flag) {
				rp_GenAmp(RP_CH_1, 0.5);
				// rp_GenAmp(RP_CH_2, 0);
				flag = 1;
				pin_write( FGTRIG, 0);
				t0 = micros();
			}
			else {
				rp_GenAmp(RP_CH_1, 1);
				// rp_GenAmp(RP_CH_2, 1.0);
				flag = 0;
				pin_write( FGTRIG, 1);
				t0 = micros();
			}
		}
	}
	pin_unexport(FGTRIG);
	rp_Release();

	return 0;
}
long micros(){
	struct timeval currentTime;
	long time;
	gettimeofday(&currentTime, NULL);
	time = currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	// if(time<0) time = time*(-1);
//	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return time;
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