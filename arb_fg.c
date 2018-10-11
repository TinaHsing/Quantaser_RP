#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "redpitaya/rp.h"

#define M_PI 3.14159265358979323846

long micros(void);

int main(int argc, char **argv){

    float sweep_time;
    int buff_size = 16384;
	long t0;
	float freq = 0.1;

    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }

    float *t = (float *)malloc(buff_size * sizeof(float));
    float *x = (float *)malloc(buff_size * sizeof(float));
    float *y = (float *)malloc(buff_size * sizeof(float));
	
	printf("enter sweep time in us: ");
	scanf("%f",&sweep_time);

	// sweep_time /= 1000000;
    for(int i = 0; i < buff_size; i++){
        t[i] = (2 * M_PI * freq)*sweep_time / buff_size * i;
    }

    for (int i = 0; i < buff_size; i++){
        x[i] = 0.2*sin(t[i]);
        y[i] = sin(t[i]);
    }
	rp_GenOutDisable(RP_CH_1);
    rp_GenWaveform(RP_CH_1, RP_WAVEFORM_ARBITRARY);
    // rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);

    rp_GenArbWaveform(RP_CH_1, x, buff_size);
    // rp_GenArbWaveform(RP_CH_2, y, buff_size);

    rp_GenAmp(RP_CH_1, 1.0);
    // rp_GenAmp(RP_CH_2, 1.0);

    rp_GenFreq(RP_CH_1, 1000000/sweep_time);
    // rp_GenFreq(RP_CH_2, freq);
	
	t0 = micros();
    rp_GenOutEnable(RP_CH_1);
    // rp_GenOutEnable(RP_CH_2);
	
	while((micros()-t0)<sweep_time);
	
	rp_GenOutDisable(RP_CH_1);
	
	// rp_GenWaveform(RP_CH_1, RP_WAVEFORM_ARBITRARY);
	rp_GenArbWaveform(RP_CH_1, y, buff_size);
	// rp_GenAmp(RP_CH_1, 1.0);
	// rp_GenFreq(RP_CH_1, 1000000/sweep_time);
	t0 = micros();
	rp_GenOutEnable(RP_CH_1);
	while((micros()-t0)<sweep_time);
	rp_GenOutDisable(RP_CH_1);
	// rp_GenOutDisable(RP_CH_2);

    /* Releasing resources */
    free(y);
    free(x);
    free(t);
    rp_Release();
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