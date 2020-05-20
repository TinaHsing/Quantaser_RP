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
#define CH RP_CH_1

int main(int argc, char **argv){

	FILE *fp;
	float sweep_time;
	long arb_size = 32768;
	float arrf[arb_size];
	double arr[arb_size];
	
	
	fp = fopen(argv[1], "rb");
	fread(arr, sizeof(double), arb_size, fp);
	fclose(fp);
	for(int i=0; i<arb_size; i++)
	{
		arrf[i] = arr[i];
	}
	
	sweep_time = atoi(argv[2]); //ms

    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
    }
	
	rp_GenAmp(CH, 0);
	rp_GenOutEnable(CH);
	
	rp_GenWaveform(CH, RP_WAVEFORM_ARBITRARY);
	printf("1\n");
	rp_GenArbWaveform(CH, arrf, arb_size);
printf("2\n");
	rp_GenFreq(CH, 1000.0/sweep_time);
		printf("3\n");
	rp_GenAmp(CH, 1);
	t_start = micros();		
	while((micros()-t_start)<sweep_time*1000){}
	rp_GenAmp(CH, 0); //chirp end
	
	
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