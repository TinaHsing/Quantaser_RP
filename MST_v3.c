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


///////*gpio pin define*/////////
#define FGTRIG 977 //DIO1_N, amplitude scan start trigger, BNC 977
////***978, 979 用在integrator, DIO2_N DIO3_N***///
#define FGTTL 980 //DIO4_N
#define TEST_TTL_0 981 ////DIO5_N
#define TEST_TTL_1 982 //DIO6_N
#define TEST_TTL_2 983 //DIO7_N
// #define TEST_TTL_3 982

/////******define DAC*******/////////
#define DAC_BIT_16
/* DAC LTC2615 */
#define DAC0_ADD 0x10
#define DAC1_ADD 0x52
#define CC 0b0011
#define ref 2.5

#ifdef DAC_BIT_14
	#define max 16383
#endif
#ifdef DAC_BIT_16
	#define max 65535
#endif	

#define CH_A 0b0000
#define CH_B 0b0001
#define CH_C 0b0010
#define CH_D 0b0011
#define CH_E 0b0100
#define CH_F 0b0101
#define CH_G 0b0110
#define CH_H 0b0111

#define DAC1 	1
#define DAC2 	2
#define DAC3 	3
#define DAC4 	4
#define DAC5 	5
#define DAC6 	6
#define DAC7 	7
#define DAC8 	8
#define DAC9 	9
#define DAC10 	10

int g_i2cFile;

///////*constant define*//////
#define UPDATE_RATE 30 //us
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1

#define START_SCAN 1
#define END_SCAN 0
////////*MMAP*///////////////
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
////////* gpio *///////////
static int pin_export(int);
static int pin_unexport(int);
static int pin_direction(int, int);
static int pin_write(int, int);

/* I2C */
void i2cOpen(void);
void i2cClose(void);
void i2cSetAddress(int);
void WriteRegisterPair(uint8_t, uint16_t);
void LTC2615_write(bool, uint8_t, float);
void DAC_out(uint8_t, float);


///////*ADC*//////////////
void ADC_init(void);
void ADC_req(uint32_t*, float*, float*);
void write_txt(uint32_t*, int, uint32_t);
void write_file(float*, int, uint32_t);


//////* r/w txt file*///////////
void writeFile(char *, int);
char readFile(char *);

//////*Address R/W*////////
void AddrWrite(unsigned long, unsigned long);
uint32_t AddrRead(unsigned long);
void map2virtualAddr(uint32_t**, uint32_t);

///////* time read*////////
long micros(void);
float adc_gain_p, adc_gain_n;
uint32_t adc_offset;
int idx=0;
float int2float(uint32_t, float, float, uint32_t);

float freq_HV, AC_amp, DC_amp;
uint32_t ramp_pts, op_count = 0;

void* map_base = (void*)(-1);

int main(int argc, char *argv[]) 
{
	int	save=0;
	long delay_ms = 0;
	uint32_t adc_counter;
	uint32_t *adc_mem = (uint32_t *)malloc(35000 * sizeof(uint32_t));
	float *adc_mem_f = (float *)malloc(35000 * sizeof(float));
	char ch;
	// char read_done;
	uint32_t *adc_idx_addr = NULL;
	uint32_t *adc_ch2 = NULL;
	FILE *fp, *fp2, *fp_log = fopen("MST_log.txt", "a");
	
	writeFile("read_done.txt", 1);
	writeFile("write_done.txt", 0);
	
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}
	fprintf(fp_log, "start\n");
	fclose(fp_log);
	map2virtualAddr(&adc_idx_addr, 0x40200064); //adc_idx
	//0x40200068 for ch1, 0x40200070 for ch2
	map2virtualAddr(&adc_ch2, 0x40200070); //adc_ch2
	
	i2cOpen();
	pin_export(FGTRIG);
	pin_export(FGTTL);
	pin_export(TEST_TTL_0);
	pin_export(TEST_TTL_1);
	pin_export(TEST_TTL_2);
	pin_direction(FGTRIG, OUT);
	pin_direction(FGTTL, OUT);
	pin_direction(TEST_TTL_0, OUT);
	pin_direction(TEST_TTL_1, OUT);
	pin_direction(TEST_TTL_2, OUT);
	pin_write( FGTRIG, 0);
	pin_write( FGTTL, 0);
	pin_write( TEST_TTL_0, 0);
	pin_write( TEST_TTL_1, 0);
	pin_write( TEST_TTL_2, 0);

	freq_HV = atof(argv[1]);	//頻率
	ramp_pts = atol(argv[2]);		//掃描點數
	fp = fopen(argv[3], "rb"); //read amp in mV
	fp2 = fopen(argv[4], "rb"); // read amp in mV
	delay_ms = atol(argv[5])*1000; //input ms convert to us
	save = atoi(argv[6]);
	adc_offset = atoi(argv[7]);
	adc_gain_p = atof(argv[8]);
	adc_gain_n = atof(argv[9]);
	
	if(fp == NULL || fp2 == NULL) {
		fp_log = fopen("MST_log.txt", "a");
		fprintf(fp_log, "bin file open fail!\n");
		fclose(fp_log);
		// printf("bin file open fail!\n"); 
		return -1;
	}
	
	float *AC_amp = (float *)malloc(ramp_pts * sizeof(float));
	float *DC_amp = (float *)malloc(ramp_pts * sizeof(float));
	
	fread(DC_amp, sizeof(float), ramp_pts, fp);
	fclose(fp);
	
	fread(AC_amp, sizeof(float), ramp_pts, fp2);
	fclose(fp2);

	for(int i=0; i<ramp_pts; i++)
	{
		AC_amp[i] /= 1000;
		DC_amp[i] /= 1000;
	}
	
	ADC_init();
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
	rp_GenFreq(RP_CH_1, freq_HV);
	rp_GenAmp(RP_CH_1, 0);
	rp_GenOutEnable(RP_CH_1);

	rp_GenAmp(RP_CH_1, AC_amp[0]);
	DAC_out(DAC8, DC_amp[0]);
	AddrWrite(0x40200064, 0);//addwrite idx
	while(1)
	{
		AddrWrite(0x40200044, START_SCAN);
		pin_write( FGTRIG, 1);	// scan start trigger
		pin_write( TEST_TTL_0, 1);
		for(int i=0; i<ramp_pts; i++) 
		{	
			
			// AddrWrite(0x40200064, i);//addwrite idx
			*adc_idx_addr = i;//addwrite idx
			rp_GenAmp(RP_CH_1, AC_amp[i]);
			DAC_out(DAC8, DC_amp[i]);
		}
		pin_write( FGTRIG, 0);
		AddrWrite(0x40200044, END_SCAN);
		adc_counter = AddrRead(0x40200060); //讀取adc_mem 目前有幾個data
		printf("adc_counter= %d\n",adc_counter); // 要<16384
		
		rp_GenAmp(RP_CH_1, 0);
		DAC_out(DAC8, DC_amp[0]);
		pin_write( TEST_TTL_0, 0);
		fp_log = fopen("MST_log.txt", "a");
		fprintf(fp_log,"%d" , op_count++);
		fclose(fp_log);
		for(int i=0; i<adc_counter; i++)
		{
			*adc_idx_addr = i;
			// AddrWrite(0x40200064, i);//addwrite idx 
			// adc_mem[i] = AddrRead(0x40200070); //read fpga adc_mem[idx], 0x40200068 for ch1, 0x40200070 for ch2
			adc_mem[i] = *adc_ch2;
			adc_mem_f[i] = int2float(*(adc_mem+i), adc_gain_p, adc_gain_n, adc_offset);
		}
		if(adc_counter == ramp_pts-1)  {
			adc_counter++;
			adc_mem_f[ramp_pts-1] = adc_mem_f[ramp_pts-2];
			// printf("%d, %f, ",ramp_pts-2, adc_mem_f[ramp_pts-2]);
			// printf("%d, %f\n ",ramp_pts-1, adc_mem_f[ramp_pts-1]);
		}
		fp_log = fopen("MST_log.txt", "a");
		fprintf(fp_log,", rd");
		fclose(fp_log);
		AddrWrite(0x4020005C, 1); //end read flag, reset adc_counter
		// for(int i=0;i<adc_counter;i++)
		// {
			// printf("%d. %f\n",i+1, adc_mem_f[i]);
		// }
		write_file(adc_mem_f, save, adc_counter);	
		
		fp_log = fopen("MST_log.txt", "a");
		fprintf(fp_log,", write_done\n");
		fclose(fp_log);
		
		fp = fopen("MST.txt","r");
		if(fp==NULL) {
			fp_log = fopen("MST_log.txt", "a");
			fprintf(fp_log, "open MST.txt fail\n");
			fclose(fp_log);
		}
		ch = getc(fp);
		fclose(fp);
		printf("%c\n", ch);
		if(ch=='1') break;
		usleep(delay_ms);
		
		// read_done = readFile("read_done.txt");
		// printf("read_done = %c\n", read_done);
		
		// if(read_done == '1') {
			// writeFile("read_done.txt", 0);
			// writeFile("write_done.txt", 1);
			// read_done = readFile("read_done.txt");
		// }
		
		// while(read_done == '0') {
			// read_done = readFile("read_done.txt");
		// }
		
	}
	fp_log = fopen("MST_log.txt", "a");
	fprintf(fp_log, "end of while loop!\n\n\n");
	AddrWrite(0x40200058, 1); //write end_write to H，此時python解鎖run 按鈕
	fclose(fp_log);
	rp_Release();
	free(adc_mem);
	free(adc_mem_f);
	free(AC_amp);
	free(DC_amp);

	pin_unexport(FGTRIG);
	pin_unexport(FGTTL);
	pin_unexport(TEST_TTL_0);
	pin_unexport(TEST_TTL_1);
	pin_unexport(TEST_TTL_2);
	return 0;
}

void map2virtualAddr(uint32_t** virt_addr, uint32_t tar_addr)
{
	int fd = -1;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, tar_addr & ~MAP_MASK);
	*virt_addr = map_base + (tar_addr & MAP_MASK);
	// printf("tar_addr : %x , ", tar_addr);
	
	close(fd);
}


void AddrWrite(unsigned long addr, unsigned long value)
{
	int fd = -1;
	void* virt_addr;
	
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

static int pin_export(int pin)
{
	char shell[MAX_PATH];
	sprintf(shell,"echo %d > /sys/class/gpio/export", pin);
	system(shell);
	return 0;
}

static int pin_unexport(int pin)
{
        char shell[MAX_PATH];
        sprintf(shell,"echo %d > /sys/class/gpio/unexport", pin);
        system(shell);

	return 0;
}

static int pin_direction(int pin, int dir){

	char shell[MAX_PATH];
	snprintf(shell, MAX_PATH, "echo %s > /sys/class/gpio/gpio%d/direction",((dir==IN)?"in":"out"),pin);
	system(shell);

	return 0;
}

static int pin_write(int pin, int value)
{
	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	// get pin value file descrptor
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Unable to to open sysfs pins value file %s for writing\n",path);
		return -1;
	}
	if(value==LOW){
		//write low
		if (1 != write(fd, "0", 1)) {
			fprintf(stderr, "Unable to write value\n");
			return -1;
		}
	}
        else if(value==HIGH){
		//write high
		if (1 != write(fd, "1", 1)) {
                	fprintf(stderr, "Unable to write value\n");
                	return -1;
		}
	}else fprintf(stderr, "Nonvalid pin value requested\n");

	//close file
	close(fd);
	return 0;
}
void ADC_init(void){
	rp_AcqReset();
	rp_AcqSetDecimation(1);
	rp_AcqStart(); //寫osc address 0x0 value 0x01=> adc_arm_do = 1
	rp_AcqSetGain(RP_CH_1, RP_HIGH); //broken
	// rp_AcqSetGain(RP_CH_2, RP_HIGH);
}
void write_txt(uint32_t* adc_data, int save, uint32_t adc_counter)
{
	char shell[MAX_PATH];
	system("touch cnt.txt");
	system("echo "" > cnt.txt");
	if(save) 
	{
		// for(int i=0;i<adc_counter;i++)
		// {
			// printf("%d. %f, %d\n",i+1, int2float(*(adc_data+i), adc_gain_p, adc_gain_n, adc_offset), *(adc_data+i));
			// sprintf(shell,"echo %f >> adc_data.txt", int2float(*(adc_data+i), adc_gain_p, adc_gain_n, adc_offset));
			// sprintf(shell,"echo %d >> cnt.txt", adc_counter);
			// system(shell);
		// }
		sprintf(shell,"echo %d >> cnt.txt", adc_counter);
		system(shell);
	}
}

void write_file(float *adc_data, int save, uint32_t adc_counter)
{
	// char read_done;
	if(save)
	{
		// read_done = readFile("read_done.txt");
		// printf("read_done = %c\n", read_done);
		// if(read_done == '1')
		// {
			// writeFile("read_done.txt", 0);
			// FILE *fp, *fp2;
			// fp = fopen("QIT_adc_data.bin", "wb");
			// fp2 = fopen("cnt.txt", "w");
			// fwrite(adc_data, sizeof(float), adc_counter, fp);
			// fprintf(fp2, "%d", adc_counter);
			// fclose(fp);
			// fclose(fp2);
			// writeFile("write_done.txt", 1);
		// }
		FILE *fp, *fp2;
		fp = fopen("QIT_adc_data.bin", "wb");
		fp2 = fopen("cnt.txt", "w");
		fwrite(adc_data, sizeof(float), adc_counter, fp);
		fprintf(fp2, "%d", adc_counter);
		fclose(fp);
		fclose(fp2);
	}	
}

void writeFile(char *fileName, int value)
{
	FILE *fp = fopen(fileName, "w");
	fprintf(fp, "%d", value);
	fclose(fp);	
}

char readFile(char *fileName)
{
	char ch;
	FILE *fp = fopen(fileName, "r");
	ch = getc(fp);
	fclose(fp);
	// printf("ch=%c\n", ch);
	return ch;
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

void ADC_req(uint32_t* buff_size, float* buff, float* adc_data) {
	// rp_AcqGetLatestDataV(RP_CH_1, buff_size, buff);
	rp_AcqGetLatestDataV(RP_CH_2, buff_size, buff);
	*(adc_data+idx) = buff[*buff_size-1];
	
	// printf("%f\n", buff[*buff_size-1]);
	// printf("%d. %f\n", idx, *(adc_data+idx));
	idx++;
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

void i2cOpen()
{
	g_i2cFile = open("/dev/i2c-0", O_RDWR);
	if (g_i2cFile < 0) {
		perror("i2cOpen");
		exit(1);
	}
}

// close the Linux device
void i2cClose()
{
	close(g_i2cFile);
}

void i2cSetAddress(int address)
{
	if (ioctl(g_i2cFile, I2C_SLAVE, address) < 0) {
		perror("i2cSetAddress");
		exit(1);
	}
}

void WriteRegisterPair(uint8_t reg, uint16_t value)
{
	uint8_t data[3];
	data[0] = reg;
	data[1] = value & 0xff;
	data[2] = (value >> 8) & 0xff;
	if (write(g_i2cFile, data, 3) != 3) {
		perror("pca9555SetRegisterPair");
	}
}

void LTC2615_write(bool sel, uint8_t ch, float value)
{
	uint8_t t[2];
	uint16_t code;
	
	code = (uint16_t)(value/ref*max);
	#ifdef DAC_BIT_14
		t[0] = (code >> 8)<<2 | ((uint8_t)code & 0b11000000)>>6; //high byte
		t[1] = (uint8_t)code << 2; //low byte
	#endif
	#ifdef DAC_BIT_16
		t[0] = code >> 8;
		t[1] = (uint8_t)code; 
	#endif
	
	if(!sel)
	{
		i2cSetAddress(DAC0_ADD);
		WriteRegisterPair((CC << 4) | ch, (uint16_t)t[1]<<8 | t[0]);
	}
	else
	{
		i2cSetAddress(DAC1_ADD);
		WriteRegisterPair((CC << 4) | ch, (uint16_t)t[1]<<8 | t[0]);
	}	
	// Wire.beginTransmission(ADD);
	// Wire.write((CC << 4) | ch);
	// Wire.write(t,2); 
	// Wire.endTransmission();
}

void DAC_out(uint8_t dac_num, float value)
{
	value = value/4.0;
	if(dac_num == DAC1) LTC2615_write(0, CH_A, value);
	else if(dac_num == DAC2) LTC2615_write(0, CH_B, value);
	else if(dac_num == DAC3) LTC2615_write(0, CH_C, value);
	else if(dac_num == DAC4) LTC2615_write(0, CH_F, value);
	else if(dac_num == DAC5) LTC2615_write(0, CH_E, value);
	else if(dac_num == DAC6) LTC2615_write(1, CH_A, value);
	else if(dac_num == DAC7) LTC2615_write(1, CH_B, value);
	else if(dac_num == DAC8) LTC2615_write(1, CH_C, value);
	else if(dac_num == DAC9) LTC2615_write(1, CH_D, value);
	else if(dac_num == DAC10) LTC2615_write(1, CH_E, value);
}