#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "redpitaya/rp.h"

float freq, amp;

int main(int argc, char **argv){

    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }
	printf("enter frequency(Hz) and amplitude(0~1V)\n");
	scanf("%f%f",&freq,&amp);
	while ( getchar() != '\n' );
    /* Generating frequency */
    rp_GenFreq(RP_CH_1, freq);

    /* Generating amplitude */
    rp_GenAmp(RP_CH_1, amp);

    /* Generating wave form */
    rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);

    /* Enable channel */
    rp_GenOutEnable(RP_CH_1);

    /* Releasing resources */
    rp_Release();

    return 0;
}