/* Red Pitaya C API example Generating continuous signal  
 * This application generates a specific signal */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "redpitaya/rp.h"

long micros(void);
int main(int argc, char **argv){
	float freq, amp;
	long t[10],t0;

	/* Print error, if rp_Init() function failed */
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}

	/* Generating frequency */
	printf("input freq in Hz: ");
	scanf("%f", &freq);
	printf("input amp in V: ");
	scanf("%f", &amp);
	rp_GenFreq(RP_CH_1, freq);

	/* Generating amplitude */
	rp_GenAmp(RP_CH_1, amp);

	/* Generating wave form */
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);

	/* Enable channel */
	rp_GenOutEnable(RP_CH_1);
	t0=micros();
	for(int i=0;i<10;i++)
	{
		freq += 1000;
		rp_GenFreq(RP_CH_1, freq);
		t[i]=micros()-t0;
		// t0 = t[i];
	}
	for(int i=0;i<10;i++)
	{
		printf("%ld\n",t[i]);
	}
	/* Releasing resources */
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