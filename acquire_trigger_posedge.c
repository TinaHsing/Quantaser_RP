/* Red Pitaya C API example Acquiring a signal from a buffer  
 * This application acquires a signal on a specific channel */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "redpitaya/rp.h"

void write_file(float *);
uint32_t buff_size = 16384;
int main(int argc, char **argv){

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


        
        float *buff = (float *)malloc(buff_size * sizeof(float));

        rp_AcqReset();
        rp_AcqSetDecimation(1);
        rp_AcqSetTriggerLevel(RP_CH_2, 0.1);
        rp_AcqSetTriggerDelay(0);

        rp_AcqStart();

        /* After acquisition is started some time delay is needed in order to acquire fresh samples in to buffer*/
        /* Here we have used time delay of one second but you can calculate exact value taking in to account buffer*/
        /*length and smaling rate*/

        sleep(1);
        rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHB_PE);
        rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
		printf("1\n");
        while(1){
                rp_AcqGetTriggerState(&state);
                if(state == RP_TRIG_STATE_TRIGGERED){
                break;
                }
        }
		printf("2\n");
        rp_AcqGetOldestDataV(RP_CH_2, &buff_size, buff);
        int i;
        for(i = 0; i < buff_size; i++){
                printf("%f\n", buff[i]);
        }
		write_file(buff);
        /* Releasing resources */
        free(buff);
        rp_Release();
        return 0;
}
        
void write_file(float *adc_data)
{
	FILE *fp;
	int i=0;
	fp = fopen("trig_data.txt", "w");
	
	for(i = 0; i < buff_size; i++){
		fprintf(fp, "%f\n", adc_data[i]);
	}
	
	fclose(fp);

}