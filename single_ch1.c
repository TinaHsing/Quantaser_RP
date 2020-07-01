#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "redpitaya/rp.h"
#define CH = RP_CH_1

float freq, amp, offset;

int main(int argc, char *argv[]){

    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }
	rp_GenAmp(CH, 0);
	freq = atof(argv[1])*1000; //KHz
	amp = atof(argv[2])/1000; //input mV convert to V
	// offset = atof(argv[3])/1000;
	rp_GenWaveform(CH, RP_WAVEFORM_SINE);
    rp_GenFreq(CH, freq);
	rp_GenOffset(CH, 0);
    rp_GenAmp(CH, amp);
    rp_GenOutEnable(CH);

    rp_Release();

    return 0;
}