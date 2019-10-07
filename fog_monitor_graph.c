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

uint32_t AddrRead(unsigned long);
void write_file(int, uint32_t *, uint32_t *, uint32_t *, uint32_t *);

unsigned long addr1=0x40000130; //error signal
unsigned long addr2=0x40000178; //1st integrator_ unlimited
unsigned long addr3=0x4000016C; //1st integrator_ after divide
unsigned long addr4=0x4000017C; //2st integrator_ after divide

int main(int argc, char *argv[])
{
	
	uint32_t buff_size = atoi(argv[1]);
	
	uint32_t *buff = (uint32_t *)malloc(buff_size * sizeof(uint32_t));
	uint32_t *buff2 = (uint32_t *)malloc(buff_size * sizeof(uint32_t));
	uint32_t *buff3 = (uint32_t *)malloc(buff_size * sizeof(uint32_t));
	uint32_t *buff4 = (uint32_t *)malloc(buff_size * sizeof(uint32_t));
	
	for(int i=0; i<buff_size; i++) 
	{
		buff[i] = AddrRead(addr1);
		buff2[i] = AddrRead(addr2);
		buff3[i] = AddrRead(addr3);
		buff4[i] = AddrRead(addr4);
		usleep(atoi(argv[2]));
	}
	write_file(buff_size, buff, buff2, buff3, buff4);
	
}

void write_file(int buff_size, uint32_t *adc_data, uint32_t *adc_data2, uint32_t *adc_data3, uint32_t *adc_data4)
{
	FILE *fp, *fp2, *fp3, *fp4;
	// int i=0;
	fp = fopen("addr1.bin", "w");
	fp2 = fopen("addr2.bin", "w");
	fp3 = fopen("addr3.bin", "w");
	fp4 = fopen("addr4.bin", "w");
	
	fwrite(adc_data, sizeof(uint32_t), buff_size, fp);
	fwrite(adc_data2, sizeof(uint32_t), buff_size, fp2);
	fwrite(adc_data3, sizeof(uint32_t), buff_size, fp3);
	fwrite(adc_data4, sizeof(uint32_t), buff_size, fp4);
	
	// fp = fopen("trig_data.txt", "w");
	// fp2 = fopen("trig_data2.txt", "w");
	
	// for(int i = 0; i < buff_size; i++){
		// fprintf(fp, "%f\n", adc_data[i]);
		// fprintf(fp2, "%f\n", adc_data2[i]);
	// }
	
	// for(int i = 0; i < buff_size; i++){
		// printf("%f, ", adc_data[i]);
		// printf("%f\n", adc_data2[i]);
	// }
	
	
	fclose(fp);
	fclose(fp2);
	fclose(fp3);
	fclose(fp4);

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