/* Red Pitaya C API example Generating signal pulse on an external trigger
* This application generates a specific signal */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "redpitaya/rp.h"

int main(int argc, char **argv){
	
	    int cnt, rp;
	    uint32_t period;

        /* Burst count */
		printf("rp_GenBurstCountt: \n");
		scanf("%d", &cnt);
		printf("rp_GenBurstRepetitions: \n");
		scanf("%d", &rp);
		printf("rp_GenBurstPeriod: \n");
		scanf("%d", &period);

        /* Print error, if rp_Init() function failed */
        if(rp_Init() != RP_OK){
                fprintf(stderr, "Rp api init failed!\n");
        }

        rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
        rp_GenFreq(RP_CH_1, 1000);
        rp_GenAmp(RP_CH_1, 1.0);

        rp_GenMode(RP_CH_1, RP_GEN_MODE_BURST);
        // rp_GenBurstCount(RP_CH_1, 1);
        // rp_GenBurstRepetitions(RP_CH_1, 10000);
        // rp_GenBurstPeriod(RP_CH_1, 5000);
        rp_GenBurstCount(RP_CH_1, cnt);
        rp_GenBurstRepetitions(RP_CH_1, rp);
        rp_GenBurstPeriod(RP_CH_1, period);
		// rp_GenTrigger(1);
        // sleep(1);
        rp_GenOutEnable(RP_CH_1);
        // rp_Release();
}
