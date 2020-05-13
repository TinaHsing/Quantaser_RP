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

///////*timing define*////////
#define TTL_WAIT 1
#define DAMPING_WAIT 1
#define ISOLATION_TIME 8 // 8ms

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
void write_txt(uint32_t*, int, uint32_t);
void write_file(float*, int, uint32_t);
void write_file_single(float*, uint32_t);
//////*Address R/W*////////
void AddrWrite(unsigned long, unsigned long);
uint32_t AddrRead(unsigned long);
///////* time read*////////
long micros(void);
float adc_gain_p, adc_gain_n;
uint32_t adc_offset;

float int2float(uint32_t, float, float, uint32_t);

float offset;
float trapping_amp;
float ramp_step;
long ramp_pts;
float final_amp;
float freq;
long t_start;
int save;
long t_now;


void* map_base = (void*)(-1);

int main(int argc, char *argv[]) 
{
	uint32_t adc_counter;
	uint32_t *adc_mem = (uint32_t *)malloc(35000 * sizeof(uint32_t));
	float *adc_mem_f = (float *)malloc(35000 * sizeof(float));
	FILE *fp_ch2;
	long arb_size = 32768;
	double arr[arb_size];
	float arrf[arb_size];
	int ttl_dura, damping_dura;
	float freq_factor, ramp_ch2=0, ramp_step2;
	
	
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}
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

/* ----- input parameters-------------- */

	freq = atof(argv[1]);	
	ttl_dura = atoi(argv[2]);//input ms
	damping_dura = atoi(argv[3]);//input ms
	trapping_amp = atof(argv[4])/1000; //input mV convert to V 
	ramp_step = atof(argv[5])/1000;//input mV convert to V 
	ramp_pts = atol(argv[6]);
	final_amp = atof(argv[7])/1000;//input mV convert to V 
	offset = atof(argv[8])/1000;//input mV convert to V 
	adc_offset = atoi(argv[9]);
	adc_gain_p = atof(argv[10]);
	adc_gain_n = atof(argv[11]);
	save = atoi(argv[12]);
	fp_ch2 = fopen(argv[13], "rb");
	freq_factor = atof(argv[14]);
	

	ADC_init();
	rp_GenOffset(RP_CH_1, offset);
	
/*---------load chirp data-----------------------------*/	
	fread(arr, sizeof(double), arb_size, fp_ch2);
	for(int i=0; i<arb_size; i++)
	{
		arrf[i] = arr[i];
	}
	fclose(fp_ch2);
	// write_file_single(arrf, arb_size);
	
/*-------trapping start-----------*/		
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
	rp_GenFreq(RP_CH_1, freq);
	rp_GenAmp(RP_CH_1, 0);
	rp_GenOutEnable(RP_CH_1);
	
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC);
	rp_GenAmp(RP_CH_2, 0);
	rp_GenOutEnable(RP_CH_2);
	
	rp_GenAmp(RP_CH_1, trapping_amp);

/*---------ch2 DC out -----------------------------*/
	
	t_start = micros();
	while((micros()-t_start)<TTL_WAIT*1000){};
	pin_write( FGTTL, 1);
	pin_write( TEST_TTL_0, 1);
	
	t_start = micros();
	while((micros()-t_start)<ttl_dura*1000){
		t_now = micros()-t_start;
		if(t_now>=DAMPING_WAIT*1000 && t_now<(DAMPING_WAIT+damping_dura)*1000)
			rp_GenAmp(RP_CH_2, 1);
		else if (t_now>=(DAMPING_WAIT+damping_dura)*1000) 
			rp_GenAmp(RP_CH_2, 0);
	}
	pin_write( FGTTL, 0);	
	
/*-------isolation -----------*/   
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
	rp_GenAmp(RP_CH_2, 0);
	rp_GenOutEnable(RP_CH_2);
	rp_GenFreq(RP_CH_2, 1000.0/ISOLATION_TIME);
	rp_GenArbWaveform(RP_CH_2, arrf, arb_size);
	
	pin_write( TEST_TTL_1, 1); //isolation trigger
	rp_GenAmp(RP_CH_2, 1);	
	usleep(ISOLATION_TIME*1000-50);
	rp_GenAmp(RP_CH_2, 0);
	
/*-------ramp -----------*/ 
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE);
	rp_GenFreq(RP_CH_2, freq_factor*freq);
	ramp_step2 = 1.0/ramp_pts;

	pin_write( TEST_TTL_2, 1); //ramp trigger
	pin_write( FGTRIG, 1);
	AddrWrite(0x40200044, START_SCAN);
	t_start = micros();	
	for(int i=0; i<ramp_pts; i++) 
	{	
		AddrWrite(0x40200064, i);//addwrite idx
		trapping_amp += ramp_step;
		ramp_ch2 += ramp_step2;
		while((micros()-t_start) < UPDATE_RATE){};
		rp_GenAmp(RP_CH_1, trapping_amp);
		rp_GenAmp(RP_CH_2, ramp_ch2);
		t_start = micros();
	}
	rp_GenAmp(RP_CH_1, final_amp);
	rp_GenAmp(RP_CH_2, 0);
	pin_write( FGTRIG, 0);
	AddrWrite(0x40200044, END_SCAN);
/*-------read ADC data -----------*/ 
	adc_counter = AddrRead(0x40200060); //讀取adc_mem 目前有幾個data
	printf("adc_count: %d\n", adc_counter);
	for(int i=0; i<adc_counter; i++)
	{
		AddrWrite(0x40200064, i);//addwrite idx 
		// printf("idx=%d, ",AddrRead(0x40200064));
		adc_mem[i] = AddrRead(0x40200070); //read fpga adc_mem[idx], 0x40200068 for ch1, 0x40200070 for ch2
		adc_mem_f[i] = int2float(*(adc_mem+i), adc_gain_p, adc_gain_n, adc_offset);
		// printf("adc_mem[%d]=%d\n",i, adc_mem[i]);
	}
	AddrWrite(0x4020005C, 1); //end read flag, reset adc_counter
	
	
	pin_unexport(FGTRIG);
	pin_unexport(FGTTL);
	pin_unexport(TEST_TTL_0);
	pin_unexport(TEST_TTL_1);
	pin_unexport(TEST_TTL_2);
			
	rp_Release();
	write_file(adc_mem_f, save, adc_counter);
	// write_txt(adc_mem, save, adc_counter);
	
	AddrWrite(0x40200058, 1); //write end_write to H，此時python解鎖run 按鈕
	free(adc_mem);
	free(adc_mem_f);
	return 0;
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
		for(int i=0;i<adc_counter;i++)
		{
			// printf("%d. %f, %d\n",i+1, int2float(*(adc_data+i), adc_gain_p, adc_gain_n, adc_offset), *(adc_data+i));
			// sprintf(shell,"echo %f >> adc_data.txt", int2float(*(adc_data+i), adc_gain_p, adc_gain_n, adc_offset));
			// sprintf(shell,"echo %d >> cnt.txt", adc_counter);
			// system(shell);
		}
		sprintf(shell,"echo %d >> cnt.txt", adc_counter);
		system(shell);
	}
}

void write_file(float *adc_data, int save, uint32_t adc_counter)
{
	if(save)
	{
		FILE *fp, *fp2;
		fp = fopen("adc_data_iso.bin", "wb");
		fp2 = fopen("cnt_iso.txt", "w");
		fwrite(adc_data, sizeof(float), adc_counter, fp);
		fprintf(fp2, "%d", adc_counter);
		fclose(fp);
		fclose(fp2);
	}	
}

void write_file_single(float *adc_data, uint32_t adc_counter)
{
	FILE *fp;
	fp = fopen("200k_data.bin", "wb");
	fwrite(adc_data, sizeof(float), adc_counter, fp);
	fclose(fp);
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