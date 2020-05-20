#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CHIRP_SWEEP_TIME 8.0

long arb_size = 32768;
FILE *fp_ch2;

void write_file_single(float*, uint32_t);

int main(int argc, char *argv[]) 
{
	float *t3 = (float *)malloc(arb_size * sizeof(float));
	float *x3 = (float *)malloc(arb_size * sizeof(float));
	
	for(long i = 0; i < arb_size; i++){
		t3[i] = (float)CHIRP_SWEEP_TIME / arb_size * i;
		x3[i] = 1.0/CHIRP_SWEEP_TIME*t3[i];
	}
	write_file_single(x3, arb_size);
}

void write_file_single(float *adc_data, uint32_t adc_counter)
{
	FILE *fp;
	fp = fopen("ramp.bin", "wb");
	fwrite(adc_data, sizeof(float), adc_counter, fp);
	fclose(fp);
}