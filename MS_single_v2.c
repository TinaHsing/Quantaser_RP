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
#define CHIRP_SWEEP_TIME 1
///////*gpio pin define*/////////
#define FGTRIG 983 
#define FGTTL 982
#define TEST_TTL_0 979
#define TEST_TTL_1 980
#define TEST_TTL_2 981
// #define TEST_TTL_3 982
///////*constant define*//////
#define UPDATE_RATE 30 //us
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1

#define START_SCAN 0
#define END_SCAN 1 
#define CLEAR 2
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

///////*ADC*//////////////
void ADC_init(void);
void ADC_req(uint32_t*, float*, float*);
void write_txt(float*, int);
//////*AddressWrite*////////
void AddrWrite(unsigned long, unsigned long);
///////* time read*////////
long micros(void);

float freq_HV, a0_HV, a1_HV, a2_HV, a_LV;
uint32_t ts_HV;
float final_freq, freq_factor;
int idx=0, ttl_dura, damping_dura;
void* map_base = (void*)(-1);

int main(int argc, char *argv[]) 
{
	float start_freq, k, m1, m2, amp, amp2=0;
	int	data_size=0, save=0, sweep_time, num=0;
	long arb_size = 16384, t_start, t_now, t_temp = 0, adc_read_start_time;
	// bool adc_read_flag=0;
	uint32_t buff_size = 2;
	float *buff = (float *)malloc(buff_size * sizeof(float));
	long tt[3];
	
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}
	pin_export(FGTRIG);
	pin_export(FGTTL);
	pin_export(TEST_TTL_0);
	pin_export(TEST_TTL_1);
	pin_export(TEST_TTL_2);
	// pin_export(TEST_TTL_3);
	pin_direction(FGTRIG, OUT);
	pin_direction(FGTTL, OUT);
	pin_direction(TEST_TTL_0, OUT);
	pin_direction(TEST_TTL_1, OUT);
	pin_direction(TEST_TTL_2, OUT);
	// pin_direction(TEST_TTL_3, OUT);
	pin_write( FGTRIG, 0);
	pin_write( FGTTL, 0);
	pin_write( TEST_TTL_0, 0);
	pin_write( TEST_TTL_1, 0);
	pin_write( TEST_TTL_2, 0);
	// pin_write( TEST_TTL_3, 0);				
	// printf("set HVFG parameters (freq_HV(Hz), ts_HV(ms), a0_HV, a1_HV, a2_HV(Volt, 0~1V)) :\n");
	// scanf("%f%u%f%f%f", &freq_HV,&ts_HV,&a0_HV,&a1_HV, &a2_HV);
	// printf("set chirping amplitude (0~10V) :\n");
	// scanf("%f",&a_LV);
	// printf("enter freq factor and chirp final freq in KHz: ");
	// scanf("%f%f", &freq_factor, &final_freq);
	// printf("save data to .txt file? yes(1), no(0) : \n");
	// scanf("%d",&save);
	// while ( getchar() != '\n' );
	freq_HV = atof(argv[1]);
	ts_HV = atol(argv[2]);
	a0_HV = atof(argv[3]);
	a1_HV = atof(argv[4]);
	a2_HV = atof(argv[5]);
	a_LV = atof(argv[6]);
	freq_factor = atof(argv[7]);
	final_freq = atof(argv[8]);
	save = atoi(argv[9]);
	ttl_dura = atoi(argv[10]);
	damping_dura = atoi(argv[11]);
	start_freq = 0.5*freq_HV/1000;
	data_size = ts_HV*1000/UPDATE_RATE;
	float *adc_data = (float *) malloc(sizeof(float)*data_size);
	ADC_init();
	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
	rp_GenFreq(RP_CH_1, freq_HV);
	rp_GenAmp(RP_CH_1, 0);
	rp_GenOutEnable(RP_CH_1);
	sweep_time = CHIRP_SWEEP_TIME;
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC);
	rp_GenAmp(RP_CH_2, 0);
	rp_GenOutEnable(RP_CH_2);

	a_LV /= 10;
	float *t3 = (float *)malloc(arb_size * sizeof(float));
	float *x3 = (float *)malloc(arb_size * sizeof(float));
	k = (final_freq - start_freq) / sweep_time;
	for(long i = 0; i < arb_size; i++){
		t3[i] = (float)sweep_time / arb_size * i;
		x3[i] = sin(2*M_PI*(start_freq*t3[i] + 0.5*k*t3[i]*t3[i]));
	}
	rp_GenArbWaveform(RP_CH_2, x3, arb_size);
	
	m1 = (a1_HV - a0_HV)/(ts_HV)/1000; //volt/us
	m2 = 1.0/(ts_HV)/1000;
	amp = a0_HV;
	rp_GenAmp(RP_CH_1, amp);
	
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
	
	
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
	rp_GenFreq(RP_CH_2, 1000.0/sweep_time);
	
	rp_GenAmp(RP_CH_2, a_LV); // chirp start
	pin_write( TEST_TTL_2, 1);
						
	t_start = micros();		
	while((micros()-t_start)<sweep_time*1000*0.9){}
	rp_GenAmp(RP_CH_2, 0); //chirp end
	
	// t_start = micros();
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE);
	rp_GenFreq(RP_CH_2, freq_factor*freq_HV);
	
	pin_write( FGTRIG, 1);	
	// pin_write( TEST_TTL_3, 1);
	
	AddrWrite(0x40200044, START_SCAN);
	t_start = micros(); // scan start	
	while((micros()-t_start)<ts_HV*1000)
	{		
		t_now = micros();
		// printf("t_now: %ld\n",(t_now));
		// printf("t_now: %ld\n",(t_temp));
		// printf("t_now - t_temp :%ld\n",(t_now - t_temp));
		if((t_now - t_temp) >= UPDATE_RATE)
		{
			// AddrWrite(0x40200004, 0x4000);
			rp_GenAmp(RP_CH_1, amp);
			rp_GenAmp(RP_CH_2, amp2);			
			amp = amp + m1*UPDATE_RATE;
			amp2 = amp2 + m2*UPDATE_RATE;
			adc_read_start_time = micros();
			while( (micros()-adc_read_start_time)<=31 ){};
			ADC_req(&buff_size, buff, adc_data);
			
			t_temp=t_now;			
			num++;
		}	
	}
	tt[0] = micros();
	AddrWrite(0x40200044, END_SCAN);
	tt[1] = micros();
	AddrWrite(0x40200044, CLEAR);
	tt[2] = micros();
	printf("num=%d\n",num);
	printf("tt[0]=%ld\n",tt[0]);
	printf("tt[1]=%ld\n",tt[1]);
	printf("tt[2]=%ld\n",tt[2]);
	// printf("tt[3]=%ld\n",tt[3]);
	amp = a2_HV;
	rp_GenAmp(RP_CH_1, amp);
	rp_GenAmp(RP_CH_2, 0);
	pin_write( FGTRIG, 0);
	pin_unexport(FGTRIG);
	pin_unexport(FGTTL);
	pin_unexport(TEST_TTL_0);
	pin_unexport(TEST_TTL_1);
	pin_unexport(TEST_TTL_2);
	// pin_unexport(TEST_TTL_3);
	free(t3);
	free(x3);			
	rp_Release();
	write_txt(adc_data, save);
	free(buff);
	
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
	
	// read_result = *((uint32_t *) virt_addr); //read
	// if(value == 1)
		// *((unsigned long *) virt_addr) = ((value<<14) | read_result); // start of write
	// else
		// *((unsigned long *) virt_addr) = ((value<<15) | read_result); // end of write
	switch(value)
	{
		case START_SCAN:
			*((unsigned long *) virt_addr) = 0x1;
		break;
		case END_SCAN:
			*((unsigned long *) virt_addr) = 0x2;
		break;
		case CLEAR:
			*((unsigned long *) virt_addr) = 0x0;
		break;
	}
	// if(value == START_SCAN)
		// *((unsigned long *) virt_addr) = 0x1; // start of write
	// else
		// *((unsigned long *) virt_addr) = 0x2; // end of write
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
	rp_AcqSetGain(RP_CH_1, RP_HIGH);
}
void write_txt(float* adc_data, int save)
{
	char shell[MAX_PATH];
	system("touch adc_data.txt");
	system("echo "" > adc_data.txt");
	if(save)
		for(int i=0;i<idx;i++)
		{
			// sprintf(shell,"echo %d_%f >> adc_data.txt",i, adc_data[i]);
			//printf("%f\n",*(adc_data+i));
			sprintf(shell,"echo %f >> adc_data.txt", *(adc_data+i));
			system(shell);
		}
}
void ADC_req(uint32_t* buff_size, float* buff, float* adc_data) {
	rp_AcqGetLatestDataV(RP_CH_1, buff_size, buff);
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