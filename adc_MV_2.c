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

////////*MMAP*///////////////
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
void* map_base = (void*)(-1);
float ADC_req(uint32_t* , float*, int);
long micros(void);
uint32_t AddrRead(unsigned long);
void AddrWrite(unsigned long, unsigned long);
int main(int argc, char **argv){
		// long time[2];
        if(rp_Init() != RP_OK){
                fprintf(stderr, "Rp api init failed!\n");
        }

		uint32_t buff_size, hold_times;
		int ch, gain;
        float *buff;
		// uint32_t EOS_flag;
		float sum = 0;

		ch = atoi(argv[1]);
		buff_size = atoi(argv[2]);
		buff = (float *)malloc(buff_size * sizeof(float));
		gain = atoi(argv[3]);
		hold_times = atoi(argv[4]);
		
        rp_AcqReset();
		rp_AcqSetDecimation(1);

        rp_AcqStart();
		rp_AcqSetGain(ch, gain); //gain : 0:LOW, 1:HIGH;   ch: 0:ch1, 1:ch2
		
		for(int i=0; i<hold_times; i++)
		{
			// time[0]=micros();
			AddrWrite(0x40200044, 1); // start integrate process
			while(!AddrRead(0x40200054)) {}; //integrate finish
			sum += ADC_req(&buff_size, buff, ch);
			// while(!AddrRead(0x40200054)) {};
			// time[1]=micros();
			// diff[i]=time[1]-time[0];
			// printf("%ld\n", time[1]-time[0]);
			
		}
		printf("%f\n", sum);
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
float ADC_req(uint32_t* buff_size, float* buff, int ch) {
	rp_AcqGetLatestDataV(ch, buff_size, buff);
	int i;
	float avg = 0;
	for(i = 0; i < *buff_size; i++){
			// printf("%f\n", buff[i]);
			avg += buff[i];
	}
	avg = avg / (float)*buff_size;
	// printf("%f\n", avg);
	return avg;
}
uint32_t AddrRead(unsigned long addr)
{
	int fd = -1;
	void* virt_addr;
	uint32_t read_result = 0;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	/* Map one page */
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~MAP_MASK);
	if(map_base == (void *) -1) FATAL;
	virt_addr = map_base + (addr & MAP_MASK);
	
	read_result = *((uint32_t *) virt_addr); //read
	
	if (map_base != (void*)(-1)) {
		if(munmap(map_base, MAP_SIZE) == -1) FATAL;
		map_base = (void*)(-1);
	}

	if (map_base != (void*)(-1)) {
		if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	}
	if (fd != -1) {
		close(fd);
	}
	return read_result;
}
void AddrWrite(unsigned long addr, unsigned long value)
{
	int fd = -1;
	void* virt_addr;
	// uint32_t read_result = 0;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	/* Map one page */
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~MAP_MASK);
	if(map_base == (void *) -1) FATAL;
	virt_addr = map_base + (addr & MAP_MASK);
	
	*((unsigned long *) virt_addr) = value;
	
	if (map_base != (void*)(-1)) {
		if(munmap(map_base, MAP_SIZE) == -1) FATAL;
		map_base = (void*)(-1);
	}

	if (map_base != (void*)(-1)) {
		if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	}
	if (fd != -1) {
		close(fd);
	}
}