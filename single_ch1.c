#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "redpitaya/rp.h"

float freq, amp, offset;

int main(int argc, char *argv[]){

    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }
	printf("enter frequency(Hz) and amplitude(0~1V)\n");
	scanf("%f%f",&freq,&amp);
	while ( getchar() != '\n' );
	rp_GenAmp(RP_CH_2, 0);
	freq = atof(argv[1])*1000; //KHz
	amp = atof(argv[2])/1000;
	offset = atof(argv[3])/1000;
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE);
    rp_GenFreq(RP_CH_2, freq);
	
	rp_GenOffset(RP_CH_2, offset);

    rp_GenAmp(RP_CH_2, amp);

    

    rp_GenOutEnable(RP_CH_2);

    rp_Release();

    return 0;
}