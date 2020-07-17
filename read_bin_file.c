
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
#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>
#include "redpitaya/rp.h"

int main(int argc, char *argv[])
{
	FILE *fp;
	long arb_size = 32768;
	float arr[arb_size];
	int t;
	
	t = atoi(argv[1]);
	fp = fopen(argv[2], "rb");
	printf("1");
	fread(arr, sizeof(float), arb_size, fp);
	printf("2");
	// for(int i=0; i<arb_size; i++) {
		// printf("%d. %f\n", i, arr[i]);
	// }
	fclose(fp);
	
	rp_GenAmp(RP_CH_2, 0);
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
	rp_GenArbWaveform(RP_CH_2, arr, arb_size);
	rp_GenFreq(RP_CH_2, 1000.0/t);
	
	rp_GenAmp(RP_CH_2, 1); // chirp start
	usleep(t*1000);
	rp_GenAmp(RP_CH_2, 0); //chirp end
}