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

int main(int argc, char **argv){

	float sweep_time_1;
    // float sweep_time_1, sweep_time_2;
	long arb_size = 16384;
	// long t_start;
	float start_freq_1, final_freq_1, k_1;
	// float start_freq_2, final_freq_2, k_2;
	// int out[arb_size];
	// int out;
	// float t, x;
	start_freq_1 = atof(argv[1]);
	final_freq_1 = atof(argv[2]);
	sweep_time_1 = atof(argv[3]);
	// start_freq_2 = atof(argv[4]);
	// final_freq_2 = atof(argv[5]);
	// sweep_time_2 = atof(argv[6]);
	
    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }
	rp_GenAmp(RP_CH_2, 1);
	rp_GenOutEnable(RP_CH_2);
	// AddrWrite(0x40200024, 8192);
    float *t = (float *)malloc(arb_size * sizeof(float));
	float *x_1 = (float *)malloc(arb_size * sizeof(float));
	// float *t2 = (float *)malloc(arb_size * sizeof(float));
	// float *x_2 = (float *)malloc(arb_size * sizeof(float));
	k_1 = (final_freq_1 - start_freq_1) / sweep_time_1;
	// k_2 = (final_freq_2 - start_freq_2) / sweep_time_2;
	for(long i = 0; i < arb_size; i++){
		t[i] = (float)sweep_time_1 / arb_size * i;
		x_1[i] = sin(2*M_PI*(start_freq_1*t[i] + 0.5*k_1*t[i]*t[i]));
		// t2[i] = (float)sweep_time_2 / arb_size * i;
		// x_2[i] = sin(2*M_PI*(start_freq_2*t2[i] + 0.5*k_2*t2[i]*t2[i]));
		// out[i] = x_1[i]*8191; 
		// t = (float)sweep_time_1 / arb_size * i;
		// x = sin(2*M_PI*(start_freq_1*t + 0.5*k_1*t*t));
		// out = (int)(x*8191.0);
		// AddrWrite(0x40200080, out);
	}
	rp_GenAmp(RP_CH_2, 0);
	// AddrWrite(0x40200024, 0);
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
	
	
	rp_GenArbWaveform(RP_CH_2, x_1, arb_size);
	rp_GenFreq(RP_CH_2, 1000/sweep_time_1);
	
	
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

 long micros(){
	struct timeval currentTime;
	long time;
	gettimeofday(&currentTime, NULL);
	time = currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	// if(time<0) time = time*(-1);
//	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return time;
}