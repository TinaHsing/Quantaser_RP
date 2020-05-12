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
#define TTL_DURA 50 //500
#define DAMPING_WAIT 1
#define DAMPING_DURA 5 //50
#define CHIRP_WAIT 10
#define SCAN_WAIT 10
#define CHIRP_SWEEP_TIME 1.2
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
void write_txt(float*, float*, int, uint32_t);
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

float freq_HV, offset;
uint32_t ramp_pts;
float ramp_step, ramp_step2, ramp_ch2;
float trapping_amp;
float final_amp;
float chirp_amp;
float final_freq, freq_factor;
float k;
int idx=0, ttl_dura, damping_dura;
void* map_base = (void*)(-1);

int main(int argc, char *argv[]) 
{
	float start_freq;
	
	int	save=0;

	long arb_size = 16384, t_start, t_now;
	float arr[arb_size];
	uint32_t adc_counter;
	uint32_t *adc_mem = (uint32_t *)malloc(arb_size * sizeof(uint32_t));
	float *adc_mem_f = (float *)malloc(arb_size * sizeof(float));
	FILE *fp_ch2;
	
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
	freq_HV = atof(argv[1]);	
	ramp_pts = atol(argv[2]);
	ramp_step = atof(argv[3])/1000;//input mV convert to V
	trapping_amp = atof(argv[4])/1000;//input mV convert to V
	final_amp = atof(argv[5])/1000;//input mV convert to V
	chirp_amp = atof(argv[6])/1000;//input mV convert to V
	freq_factor = atof(argv[7])/2;//fpga修改過counter step，這裡修正回來
	final_freq = atof(argv[8]);//KHz
	save = atoi(argv[9]);
	ttl_dura = atoi(argv[10]);//input ms
	damping_dura = atoi(argv[11]);//input ms
	adc_offset = atoi(argv[12]);
	offset = atof(argv[13])/1000;//input mV convert to V
	adc_gain_p = atof(argv[14]);
	adc_gain_n = atof(argv[15]);
	
	ADC_init();
	
/*---------chirp data preparation-----------------------------*/
	
	float *t3 = (float *)malloc(arb_size * sizeof(float));
	float *x3 = (float *)malloc(arb_size * sizeof(float));
	start_freq = 0.5*freq_HV/1000;
	k = (final_freq - start_freq) / CHIRP_SWEEP_TIME;
	for(long i = 0; i < arb_size; i++){
		t3[i] = (float)CHIRP_SWEEP_TIME / arb_size * i;
		x3[i] = sin(2*M_PI*(start_freq*t3[i] + 0.5*k*t3[i]*t3[i]));
	}
	// write_txt(t3, x3, 1, arb_size);
	write_file_single(x3, arb_size);
	fp_ch2 = fopen("arb.bin", "rb");
	fread(arr, sizeof(float), arb_size, fp_ch2);
	fclose(fp_ch2);
	
/*---------ch2 DC out -----------------------------*/
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
	rp_GenFreq(RP_CH_1, freq_HV);
	rp_GenAmp(RP_CH_1, 0);
	rp_GenOutEnable(RP_CH_1);
	rp_GenOffset(RP_CH_1, offset);

	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC);
	rp_GenAmp(RP_CH_2, 0);
	rp_GenOutEnable(RP_CH_2);
	
	rp_GenAmp(RP_CH_1, trapping_amp);
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
	pin_write( TEST_TTL_1, 1);
	pin_write( FGTTL, 0);
	
/*---------ch2 chirp out -----------------------------*/	
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
	
	
	rp_GenArbWaveform(RP_CH_2, arr, arb_size);
	rp_GenFreq(RP_CH_2, 1000.0/(CHIRP_SWEEP_TIME*2));
	// rp_GenFreq(RP_CH_2, 3814.7);
	printf("%d\n", AddrRead(0x40200030));
	
	
	rp_GenAmp(RP_CH_2, chirp_amp); // chirp start
	pin_write( TEST_TTL_2, 1);
						
	t_start = micros();		
	while((micros()-t_start)<1000000){}
	rp_GenAmp(RP_CH_2, 0); //chirp end
	
/*---------ch1 and ch2 ramp -----------------------------*/	
	
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE);
	rp_GenFreq(RP_CH_2, freq_factor*freq_HV);
	
	pin_write( FGTRIG, 1);	
	ramp_ch2 = 0;
	ramp_step2 = 1.0/ramp_pts;
	AddrWrite(0x40200044, START_SCAN);
	t_start = micros(); // scan start	
	for(int i=0; i<ramp_pts; i++) 
	{		
		AddrWrite(0x40200064, i);//addwrite idx
		trapping_amp += ramp_step;
		ramp_ch2 += ramp_step2;
		while((micros() - t_start) < UPDATE_RATE){}; 	
		rp_GenAmp(RP_CH_1, trapping_amp);
		rp_GenAmp(RP_CH_2, ramp_ch2);		
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
		adc_mem[i] = AddrRead(0x40200070); //read fpga adc_mem[idx], 0x40200068 for ch1, 0x40200070 for ch2
		adc_mem_f[i] = int2float(*(adc_mem+i), adc_gain_p, adc_gain_n, adc_offset);
	}
	AddrWrite(0x4020005C, 1); //end read flag, reset adc_counter
	
	pin_unexport(FGTRIG);
	pin_unexport(FGTTL);
	pin_unexport(TEST_TTL_0);
	pin_unexport(TEST_TTL_1);
	pin_unexport(TEST_TTL_2);
	free(t3);
	free(x3);			
	rp_Release();
	write_file(adc_mem_f, save, adc_counter);
	// write_txt(adc_mem, save, adc_counter);
	AddrWrite(0x40200058, 1); //write end_write to H，此時python解鎖run 按鈕
	// free(buff);
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
void write_txt(float* t, float* adc_data, int save, uint32_t adc_counter)
{
	
	if(save) 
	{
		FILE *fp;
		fp = fopen("arb.txt","w");
		for(int i=0;i<adc_counter;i++)
		{
			// printf("%d. %f, %d\n",i+1, int2float(*(adc_data+i), adc_gain_p, adc_gain_n, adc_offset), *(adc_data+i));
			// sprintf(shell,"echo %f >> adc_data.txt", int2float(*(adc_data+i), adc_gain_p, adc_gain_n, adc_offset));
			// sprintf(shell,"echo %d >> cnt.txt", adc_counter);		
			fprintf(fp, "%f, %f\n", t[i], adc_data[i]);
			// sprintf(shell,"echo %f >> arb.txt", adc_data[i]);
			// system(shell);

		}
		fclose(fp);
		// sprintf(shell,"echo %d >> cnt.txt", adc_counter);
		// system(shell);
	}
}

void write_file(float *adc_data, int save, uint32_t adc_counter)
{
	if(save)
	{
		FILE *fp, *fp2;
		fp = fopen("adc_data.bin", "wb");
		fp2 = fopen("cnt.txt", "w");
		fwrite(adc_data, sizeof(float), adc_counter, fp);
		fprintf(fp2, "%d", adc_counter);
		fclose(fp);
		fclose(fp2);
	}	
}

void write_file_single(float *adc_data, uint32_t adc_counter)
{
	FILE *fp;
	fp = fopen("arb.bin", "wb");
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