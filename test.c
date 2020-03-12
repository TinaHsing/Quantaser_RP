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

//monitor
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
void* map_base = (void*)(-1);

// static uint32_t AddrRead(unsigned long);
long micros(void);

uint32_t i=0;
long t1, t2;

int main(int argc, char *argv[])
{
	// printf("data : %d\n", AddrRead(address));
	// printf("data : %x\n", AddrRead(address));
	// printf("data : %d\n", AddrRead(0x40000004));
	// printf("data : %d\n", AddrRead(0x40000008));
	t1 = micros();
	while(1)
	{
		t2 = micros();
		if((t2-t1)>=1000000)
		{
			printf("i=%d\n", i);
			i++;
			t1 = t2;
		}	
		if(i==600) break;
	}
	
	return 0;
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
// static uint32_t AddrRead(unsigned long addr)
// {
	// int fd = -1;
	// void* virt_addr;
	// uint32_t read_result = 0;
	// if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	// /* Map one page */
	// map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~MAP_MASK);
	// if(map_base == (void *) -1) FATAL;
	// virt_addr = map_base + (addr & MAP_MASK);
	
	// read_result = *((uint32_t *) virt_addr); //read

	// if (map_base != (void*)(-1)) {
		// if(munmap(map_base, MAP_SIZE) == -1) FATAL;
		// map_base = (void*)(-1);
	// }

	// if (map_base != (void*)(-1)) {
		// if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	// }
	// if (fd != -1) {
		// close(fd);
	// }
	// return read_result;
// }