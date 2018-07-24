#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include "redpitaya/rp.h"
//#include "C:\Users\Quantaser\Dropbox\projects\MassSpectrometer\rp.h"


//global vars//
/*1. function gen and ADC*/

int main(int argc, char **argv){

	/* Print error, if rp_Init() function failed */
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}

	/* Generating frequency */
	rp_GenFreq(RP_CH_1, 1000.0);

	/* Generating amplitude */
	rp_GenAmp(RP_CH_1, 0.5);

	/* Generating wave form */
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);

	/* Enable channel */
	rp_GenOutEnable(RP_CH_1);

	/* Releasing resources */
//	rp_Release();
	
	sleep(1);
//	if(rp_Init() != RP_OK){
//		fprintf(stderr, "Rp api init failed!\n");
//	}

	/* Generating frequency */
	rp_GenFreq(RP_CH_1, 500.0);

	/* Generating amplitude */
	rp_GenAmp(RP_CH_1, 1);

	/* Generating wave form */
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);

	/* Enable channel */
	rp_GenOutEnable(RP_CH_1);

	/* Releasing resources */
	rp_Release();

	return 0;
}
 

