#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "redpitaya/rp.h"
#include <sys/time.h>
void ADC_req(uint32_t* , float*, int );
long micros(void);

// FILE *fp = fopen(DC_ADC.txt, "a");
int idx = 0;
void writeFile(char *, float);
int main(int argc, char **argv){
		// long time[2];
        if(rp_Init() != RP_OK){
                fprintf(stderr, "Rp api init failed!\n");
        }

		uint32_t buff_size;
		int ch, gain;
        float *buff;
		
		

		ch = atoi(argv[1]);
		buff_size = atoi(argv[2]);
		buff = (float *)malloc(buff_size * sizeof(float));
		gain = atoi(argv[3]);
		
        rp_AcqReset();
		rp_AcqSetDecimation(1);

        rp_AcqStart();
		rp_AcqSetGain(ch, gain); //gain : 0:LOW, 1:HIGH;   ch: 0:ch1, 1:ch2
		while(1)
		{
		// for(int i=0; i<64; i++)
		// {
			// time[0]=micros();
			ADC_req(&buff_size, buff, ch);
			usleep(1000);
			if(idx==1000) 
			{
				idx = 0;
				remove(DC_ADC.txt);
			}
			// time[1]=micros();
			// diff[i]=time[1]-time[0];
			// printf("%ld\n", time[1]-time[0]);
			
		// }
		}
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
void writeFile(char *fileName, float value)
{
	FILE *fp = fopen(fileName, "a");
	fprintf(fp, "%f", value);
	fclose(fp);	
}

void ADC_req(uint32_t* buff_size, float* buff, int ch) {
	rp_AcqGetLatestDataV(ch, buff_size, buff);
	int i;
	float avg = 0;
        for(i = 0; i < *buff_size; i++){
                // printf("%f\n", buff[i]);
				avg += buff[i];
        }
	avg = avg / (float)*buff_size;
	// fprintf(fp, "%f\n", avg);
	// fclose(fp);
	
	writeFile("DC_ADC.txt", avg);
	idx++;
	printf("%f\n", avg);
}