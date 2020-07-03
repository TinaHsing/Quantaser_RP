//////*header include*//////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h> 
#include <termios.h> 
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>
#include "redpitaya/rp.h"

/*-------MMAP---------*/
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

/*-------Address R/W---------*/
void map2virtualAddr(uint32_t**, uint32_t);

/*-------ADC data calibration---------*/
float int2float(uint32_t, float, float, uint32_t);

/*-------ADC input channel select--------*/
#define ADC_CH1

/*-------FPGA register address--------*/
#define ADC_INPUT_CH1 	0x40200068
#define ADC_INPUT_CH2 	0x40200070
#define ADC_IDX 	0x40200064
#define START_SCAN  0x40200044
#define ADC_COUNTER 0x40200060
#define END_READ 	0x4020005C
/*---------------*/
#define CONTINUE

int main(int argc, char **argv){
	
	uint32_t *adc_idx_addr = NULL;
	uint32_t *adc_ch = NULL;
	uint32_t *start_scan = NULL;
	uint32_t *adc_counter = NULL;
	uint32_t *end_read = NULL;
	uint32_t mv_num;
	float sum = 0, mv_data;
	float adc_gain_p, adc_gain_n;
	uint32_t adc_offset;
			
	mv_num = atoi(argv[1]);
	adc_offset = atoi(argv[2]);
	adc_gain_p = atof(argv[3]);
	adc_gain_n = atof(argv[4]);
	
	uint32_t *adc_mem = (uint32_t *)malloc(mv_num * sizeof(uint32_t));
	float *adc_mem_f = (float *)malloc(mv_num * sizeof(float));
	
	if(rp_Init() != RP_OK)
	{
		fprintf(stderr, "Rp api init failed!\n");
	}
	
	map2virtualAddr(&adc_idx_addr, ADC_IDX);
	map2virtualAddr(&start_scan, START_SCAN);
	map2virtualAddr(&adc_counter, ADC_COUNTER);
	map2virtualAddr(&end_read, END_READ);
	
	#ifdef ADC_CH2
		map2virtualAddr(&adc_ch, ADC_INPUT_CH2); 
	#else
		map2virtualAddr(&adc_ch, ADC_INPUT_CH1); 
	#endif
	
	#ifdef CONTINUE
	while(1)
	{
	#endif
		/*-------save adc data to fpga memory------*/
		*start_scan = 1; 
		for(int i=0; i<mv_num; i++) {
			*adc_idx_addr = i;//addwrite idx
			usleep(1000);
		}
		*start_scan = 0; 
		/*******************************************/
		printf("adc_counter = %d\n", *adc_counter);
		
		/*-------read adc data and moving average------*/
		for(int i=0; i<*adc_counter; i++)
		{
			*adc_idx_addr = i;
			adc_mem[i] = *adc_ch;
			adc_mem_f[i] = int2float(*(adc_mem+i), adc_gain_p, adc_gain_n, adc_offset);
			sum += adc_mem_f[i];
		}
		mv_data = sum/(*adc_idx_addr);
		printf("mv data = %f\n", mv_data);
		*end_read = 1; //reset adc_counter	
	#ifdef CONTINUE	
	sum = 0;
	}
	#endif
	
	rp_Release();
	return 0;
}

void map2virtualAddr(uint32_t** virt_addr, uint32_t tar_addr)
{
	int fd = -1;
	void* map_base = (void*)(-1);
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, tar_addr & ~MAP_MASK);
	*virt_addr = map_base + (tar_addr & MAP_MASK);
	// printf("tar_addr : %x , ", tar_addr);
	
	close(fd);
}
float int2float(uint32_t in, float gain_p, float gain_n, uint32_t adc_offset) {
	float adc;
	in += adc_offset; //set gain_p and gain_n to 1, add 50 ohm terminal, check offset value 
	if(in>16384) in-=16384;
	if((in>>13)>=1)
		adc = -1*gain_n*((~(in-1))& 0x3fff)/8192.0;
	else adc = gain_p*in/8191.0;
	
	return adc;
}