#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
//#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <math.h>
#include <sys/time.h>
#include "redpitaya/rp.h"

#define M_PI 3.14159265358979323846
#define WAIT 10

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
 
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

void* map_base = (void*)(-1);


long micros(void);
void AddrWrite(unsigned long, unsigned long);
uint32_t AddrRead(unsigned long);
long t_start;
#define CH RP_CH_2

int main(int argc, char **argv){

	float sweep_time;
	long arb_size;
	float freq1;
	float freq2;
	sweep_time = atoi(argv[1]); //ms
	arb_size = atoi(argv[2]);
	freq1 = atof(argv[3]); //KHz
	freq2 = atof(argv[4]);
	// arb_size = (int)(size*1000.0/0.25);

    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }
	rp_GenAmp(CH, 0);
	rp_GenOutEnable(CH);
    float *t = (float *)malloc(arb_size * sizeof(float));
	float *x = (float *)malloc(arb_size * sizeof(float));
	float *x_1 = (float *)malloc(arb_size * sizeof(float));
	float *x_2 = (float *)malloc(arb_size * sizeof(float));

	for(long i = 0; i < arb_size; i++){
		t[i] = (float)sweep_time/arb_size * i;
		x_1[i] = sin(2*M_PI*freq1*t[i]);
		x_2[i] = sin(2*M_PI*freq2*t[i]);
		x[i] = (x_1[i] + x_2[i])/2;
		// x[i] = x_1[i];
		printf("%f\n",x[i]);
	}

	
	rp_GenWaveform(CH, RP_WAVEFORM_ARBITRARY);
	
	rp_GenFreq(CH, 1000.0/sweep_time);
	AddrWrite(0x4020008c, arb_size);
	// rp_GenFreq(CH, 7629.39);
	printf("%d\n", AddrRead(0x40200030));
	
	rp_GenArbWaveform(CH, x, arb_size);
	rp_GenAmp(CH, 1);
	
	t_start = micros();		
	while((micros()-t_start)<sweep_time*1000){}
	rp_GenAmp(CH, 0); //chirp end
	
	// int i = 0;
	// while(1)
	// {
		
			// buf[i] = AddrRead(0x40200084);
			// i++;
		
		// if(i==10) break;
	// }
	// for(i=0; i<10; i++) printf("%d : %d\n", i, buf[i]);
	// rp_GenFreq(RP_CH_2, 1000/sweep_time_1);
	
	
    // rp_GenAmp(RP_CH_2, 1.0);
	// t_start = micros();		
	// while((micros()-t_start)<sweep_time_1*1000){}
	// rp_GenAmp(RP_CH_2, 0); //chirp end
	
	// t_start = micros();		
	// while((micros()-t_start)<WAIT*1000){}
	
	// rp_GenArbWaveform(RP_CH_2, x_2, arb_size);
	// rp_GenFreq(RP_CH_2, 1000/sweep_time_2);
	
	// rp_GenAmp(RP_CH_2, 1.0);
	// t_start = micros();		
	// while((micros()-t_start)<sweep_time_2*1000){}
	// rp_GenAmp(RP_CH_2, 0); //chirp end
	
    free(x_1);
	free(x_2);
	free(x);
    free(t);
	// free(x_2);
    // free(t2);
    rp_Release();
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

 long micros(){
	struct timeval currentTime;
	long time;
	gettimeofday(&currentTime, NULL);
	time = currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	// if(time<0) time = time*(-1);
//	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return time;
}