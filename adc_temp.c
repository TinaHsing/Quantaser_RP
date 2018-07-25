/* Red Pitaya C API example Acquiring a signal from a buffer
 * This application acquires a signal on a specific channel */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "redpitaya/rp.h"
#include <sys/time.h>
void ADC_req(uint32_t* , float* );
long micros(void);
int main(int argc, char **argv){
		long diff[64], time[2];
        /* Print error, if rp_Init() function failed */
        if(rp_Init() != RP_OK){
                fprintf(stderr, "Rp api init failed!\n");
        }

        /*LOOB BACK FROM OUTPUT 2 - ONLY FOR TESTING*/
        rp_GenReset();
        rp_GenFreq(RP_CH_1, 20000.0);
        rp_GenAmp(RP_CH_1, 1.0);
        rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
        rp_GenOutEnable(RP_CH_1);


        uint32_t buff_size = 16384;
        float *buff = (float *)malloc(buff_size * sizeof(float));

        rp_AcqReset();
        rp_AcqSetDecimation(1);
        // rp_AcqSetTriggerLevel(RP_CH_1, 0.1); //Trig level is set in Volts while in SCPI
        // rp_AcqSetTriggerDelay(0);

        rp_AcqStart();

        // /* After acquisition is started some time delay is needed in order to acquire fresh samples in to buffer*/
        // /* Here we have used time delay of one second but you can calculate exact value taking in to account buffer*/
        // /*length and smaling rate*/

        // sleep(1);
        // rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
        // rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

        // while(1){
                // rp_AcqGetTriggerState(&state);
                // if(state == RP_TRIG_STATE_TRIGGERED){
                // break;
                // }
        // }

        // rp_AcqGetOldestDataV(RP_CH_1, &buff_size, buff);
		
		// while(1)
		// {
			// rp_AcqGetLatestDataV(RP_CH_1, &buff_size, buff);
			// printf("%f, %f\n", buff[0],buff[buff_size-1]);
		for(int i=0; i<64; i++)
		{
			time[0]=micros();
			ADC_req(&buff_size, buff);
			time[1]=micros();
			diff[i]=time[1]-time[0];
		}
		for(int i = 0; i < 64; i++){
                printf("%d, diff=%ld\n",i, diff[i]);
        }	
		// }
        // int i;
        // for(i = 0; i < buff_size; i++){
                // printf("%f\n", buff[i]);
        // }
        // /* Releasing resources */
        free(buff);
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
void ADC_req(uint32_t* buff_size, float* buff) {
	rp_AcqGetLatestDataV(RP_CH_1, buff_size, buff);
	// printf("%f\n",buff[*buff_size-1]);
}