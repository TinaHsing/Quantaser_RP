/* Red Pitaya C API example Generating continuous signal  
 * This application generates a specific signal */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "redpitaya/rp.h"

int main(int argc, char **argv){

	system("cat /opt/redpitaya/fpga/classic/fpga.bit > /dev/xdevcfg");
	/* Print error, if rp_Init() function failed */
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}

	/* Generating frequency */
	rp_GenFreq(RP_CH_1, 300000.0);
	rp_GenFreq(RP_CH_2, 200000.0);

	/* Generating amplitude */
	rp_GenAmp(RP_CH_1, 1);
	rp_GenAmp(RP_CH_2, 1);

	/* Generating wave form */
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE);

	/* Enable channel */
	rp_GenOutEnable(RP_CH_1);
	rp_GenOutEnable(RP_CH_2);

	/* Releasing resources */
	rp_Release();

	return 0;
}
