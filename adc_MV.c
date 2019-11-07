#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "redpitaya/rp.h"
#include <sys/time.h>
void ADC_req(uint32_t* , float*, int );
long micros(void);
int main(int argc, char **argv){
		// long time[2];
        if(rp_Init() != RP_OK){
                fprintf(stderr, "Rp api init failed!\n");
        }

		uint32_t buff_size, hold_times;
		int ch, gain;
        float *buff;

		ch = atoi(argv[1]);
		buff_size = atoi(argv[2]);
		buff = (float *)malloc(buff_size * sizeof(float));
		gain = atoi(argv[3]);
		hold_times = atoi(argv[2]);
		
        rp_AcqReset();
		rp_AcqSetDecimation(1);

        rp_AcqStart();
		rp_AcqSetGain(ch, gain); //gain : 0:LOW, 1:HIGH;   ch: 0:ch1, 1:ch2
		// while(1)
		// {
		// for(int i=0; i<64; i++)
		// {
			// time[0]=micros();
			ADC_req(&buff_size, buff, ch, hold_times);
			// time[1]=micros();
			// diff[i]=time[1]-time[0];
			// printf("%ld\n", time[1]-time[0]);
			
		// }
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
void ADC_req(uint32_t* buff_size, float* buff, int ch, uint32_t hold_times) {
	rp_AcqGetLatestDataV(ch, buff_size, buff);
	int i, j;
	float avg = 0, sum = 0;
	for(j=0; j<hold_times; j++) {
        for(i = 0; i < *buff_size; i++){
                // printf("%f\n", buff[i]);
				avg += buff[i];
        }
		avg = avg / (float)*buff_size;
		sum += avg;
		printf("%f\n", avg);
	}
	printf("%f\n", sum);
}