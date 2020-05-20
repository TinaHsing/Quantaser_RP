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
#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>
#include "redpitaya/rp.h"

#define CHIRP_SWEEP_TIME 8.0

long arb_size = 32768;
FILE *fp_ch2;

void write_file_single(float*, uint32_t);

int main(int argc, char *argv[]) 
{
	float *t3 = (float *)malloc(arb_size * sizeof(float));
	float *x3 = (float *)malloc(arb_size * sizeof(float));
	
	for(long i = 0; i < arb_size; i++){
		t3[i] = (float)CHIRP_SWEEP_TIME / arb_size * i;
		if(i<16384)
			x3[i] = 1.0/CHIRP_SWEEP_TIME*t3[i];
		else 
			x3[i] = -1.0/CHIRP_SWEEP_TIME*t3[i]/2;
	}
	write_file_single(x3, arb_size);
}

void write_file_single(float *adc_data, uint32_t adc_counter)
{
	FILE *fp;
	fp = fopen("ramp.bin", "wb");
	fwrite(adc_data, sizeof(float), adc_counter, fp);
	fclose(fp);
}