/* Red Pitaya C API example Generating continuous signal  
 * This application generates a specific signal */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "redpitaya/rp.h"

#define updateRate 30 //us

long micros(void);
int main(int argc, char **argv){
	float freq;
	long t0, t_now;
	bool flag=0;

	/* Print error, if rp_Init() function failed */
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}

	/* Generating frequency */
	printf("input freq in Hz: ");
	scanf("%f", &freq);
	// printf("input amp in V: ");
	// scanf("%f", &amp);
	rp_GenFreq(RP_CH_1, freq);
	rp_GenFreq(RP_CH_2, freq);

	/* Generating amplitude */
	rp_GenAmp(RP_CH_1, 0);
	rp_GenAmp(RP_CH_2, 1);

	/* Generating wave form */
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SQUARE);
	/* Enable channel */
	rp_GenOutEnable(RP_CH_1);
	rp_GenOutEnable(RP_CH_2);
	t0=micros();
	while(1)
	{
		t_now = micros()-t0;
		if(t_now > updateRate)
		{
			if(!flag) {
				rp_GenAmp(RP_CH_1, 0.5);
				flag = 1;
				t0 = micros();
			}
			else {
				rp_GenAmp(RP_CH_1, 1);
				flag = 0;
				t0 = micros();
			}
		}
	}
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