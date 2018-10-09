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

long micros(void);
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