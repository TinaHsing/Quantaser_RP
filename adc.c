#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "redpitaya/rp.h"

float freq, amp, offset;
void write_file(int, float *, float *);

int main(int argc, char *argv[]){

	/*
	RP_DEC_1,     //!< Sample rate 125Msps; Buffer time length 131us; Decimation 1
    RP_DEC_8,     //!< Sample rate 15.625Msps; Buffer time length 1.048ms; Decimation 8
    RP_DEC_64,    //!< Sample rate 1.953Msps; Buffer time length 8.388ms; Decimation 64
    RP_DEC_1024,  //!< Sample rate 122.070ksps; Buffer time length 134.2ms; Decimation 1024
    RP_DEC_8192,  //!< Sample rate 15.258ksps; Buffer time length 1.073s; Decimation 8192
    RP_DEC_65536  //!< Sample rate 1.907ksps; Buffer time length 8.589s; Decimation 65536
	*/
	int samp_rate, trig_src, gain;
	uint32_t buff_size;
	float trig_level;
	FILE *fp;
	
	
    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }
	// printf("enter frequency(Hz) and amplitude(0~1V)\n");
	// scanf("%f%f",&freq,&amp);
	// while ( getchar() != '\n' );
	
	samp_rate = 1; //0~5
	trig_src = atoi(argv[1]); //0 or 1
	buff_size = atoi(argv[2]); // 1 ~ 16384
	trig_level = atof(argv[3]); // -1 ~ 1
	gain = atoi(argv[4]); // 0(RP_LOW) or 1(RP_HIGH)
	
	float *buff = (float *)malloc(buff_size * sizeof(float));
	float *buff2 = (float *)malloc(buff_size * sizeof(float));

	/**************** adc *************////
	rp_AcqReset();
	
	rp_AcqSetGain(RP_CH_1, gain);
	rp_AcqSetGain(RP_CH_2, gain);
	
	rp_AcqSetDecimation(samp_rate);
	rp_AcqSetTriggerLevel(trig_src, trig_level);
	rp_AcqSetTriggerDelay(0);
	rp_AcqStart();
	sleep(1);
	if(trig_src == RP_CH_1) 
		rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
	else if(trig_src == RP_CH_2)
		rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHB_PE);
	else 
		rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
	
	rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
	
	while(1)
	{
		rp_AcqGetTriggerState(&state);
		if(state == RP_TRIG_STATE_TRIGGERED){
			break;
		}
	}
	fp = fopen("trig_pass", "w");
	fclose(fp);
	rp_AcqGetOldestDataV(RP_CH_2, &buff_size, buff2);
	rp_AcqGetOldestDataV(RP_CH_1, &buff_size, buff);
	
	write_file(buff_size, buff, buff2);
	
    /* Releasing resources */
    rp_Release();

    return 0;
}

void write_file(int buff_size, float *adc_data, float *adc_data2)
{
	FILE *fp, *fp2;
	// int i=0;
	fp = fopen("trig_data.bin", "w");
	fp2 = fopen("trig_data2.bin", "w");
	
	fwrite(adc_data, sizeof(float), buff_size, fp);
	fwrite(adc_data2, sizeof(float), buff_size, fp2);
	
	// fp = fopen("trig_data.txt", "w");
	// fp2 = fopen("trig_data2.txt", "w");
	
	// for(int i = 0; i < buff_size; i++){
		// fprintf(fp, "%f\n", adc_data[i]);
		// fprintf(fp2, "%f\n", adc_data2[i]);
	// }
	
	// for(int i = 0; i < buff_size; i++){
		// printf("%f, ", adc_data[i]);
		// printf("%f\n", adc_data2[i]);
	// }
	
	
	fclose(fp);
	fclose(fp2);

}