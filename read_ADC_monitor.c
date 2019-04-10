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
#include <math.h>
#include "redpitaya/rp.h"


///////*constant define*//////
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
// #define CONTINUE
//monitor
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

static uint32_t AddrRead(unsigned long);

long micros(void);
//monitor
void* map_base = (void*)(-1);

uint32_t address = 0x4000012C;


int main(int argc, char *argv[])
{
	long delay_us;
	long t1, t2;
	long adc, min=8191, max=0;
	char shell[MAX_PATH];
	system("touch adc_data.txt");
	system("echo "" > adc_data.txt");
	delay_us = atol(argv[1]);
	t1=micros();
	for(int i=0; i<100; i++) 
	{
		// sprintf(data,"%d", AddrRead(address));
		// printf("%d\n", AddrRead(address));
		adc = AddrRead(address);
		if((adc>>13)==1) 
		{
			// adc = -1*(~(adc-1));
			adc = -10;
		}
		sprintf(shell,"echo %ld >> adc_data.txt", adc);
		system(shell);
		if(adc < min) min = adc;
		if(adc > max) max = adc;
		usleep(delay_us);
	}
	t2=micros();
	printf("%ld\n", t2-t1);
	printf("min = %ld\n", min);
	printf("max = %ld\n", max);
	printf("diff = %ld\n", max-min);
	return 0;
}

static uint32_t AddrRead(unsigned long addr)
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
	// printf("read= %d\n", read_result);
	

	// switch(value)
	// {
		// case START_SCAN:
			// *((unsigned long *) virt_addr) = 0x1;
		// break;
		// case END_SCAN:
			// *((unsigned long *) virt_addr) = 0x2;
		// break;
		// case CLEAR:
			// *((unsigned long *) virt_addr) = 0x0;
		// break;
	// }
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
	if(time<0) time += 2147483648;
//	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return time;
}
