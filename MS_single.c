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
#define FGTRIG 977
#define FGTTL 978 
#define TEST_TTL_0 979
#define TEST_TTL_1 980
#define TEST_TTL_2 981
#define TEST_TTL_3 982
///////*constant define*//////
#define UPDATE_RATE 30 //us
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1

////////* gpio *///////////
static int pin_export(int);
static int pin_unexport(int);
static int pin_direction(int, int);
static int pin_write(int, int);

///////*ADC*//////////////
void ADC_init(void);
void ADC_req(uint32_t*, float*, float*);
void write_txt(float*, int);

long micros(void);

float freq_HV, a0_HV, a1_HV, a2_HV, a_LV;
uint32_t ts_HV;
float final_freq, freq_factor;
int idx=0;

int main(void) 
{
	float start_freq, k, m1, m2, amp, amp2=0;
	int	data_size=0, save=0, sweep_time, num=0;
	long arb_size = 16384, t_start, t_now, t_temp[2] = {0,0};
	bool fg_flag=1;
	uint32_t buff_size = 2;
	float *buff = (float *)malloc(buff_size * sizeof(float));
	
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}
	pin_export(FGTRIG);
	pin_export(FGTTL);
	pin_export(TEST_TTL_0);
	pin_export(TEST_TTL_1);
	pin_export(TEST_TTL_2);
	pin_export(TEST_TTL_3);
	pin_direction(FGTRIG, OUT);
	pin_direction(FGTTL, OUT);
	pin_direction(TEST_TTL_0, OUT);
	pin_direction(TEST_TTL_1, OUT);
	pin_direction(TEST_TTL_2, OUT);
	pin_direction(TEST_TTL_3, OUT);
	pin_write( FGTRIG, 0);
	pin_write( FGTTL, 0);
	pin_write( TEST_TTL_0, 0);
	pin_write( TEST_TTL_1, 0);
	pin_write( TEST_TTL_2, 0);
	pin_write( TEST_TTL_3, 0);				
	printf("set HVFG parameters (freq_HV(Hz), ts_HV(ms), a0_HV, a1_HV, a2_HV(Volt, 0~1V)) :\n");
	scanf("%f%u%f%f%f", &freq_HV,&ts_HV,&a0_HV,&a1_HV, &a2_HV);
	printf("set chirping amplitude (0~10V) :\n");
	scanf("%f",&a_LV);
	printf("enter freq factor and chirp final freq in KHz: ");
	scanf("%f%f", &freq_factor, &final_freq);
	printf("save data to .txt file? yes(1), no(0) : \n");
	scanf("%d",&save);
	
	while ( getchar() != '\n' );
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
	while((micros()-t_start)<TTL_DURA*1000){
		t_now = micros()-t_start;
		if(t_now>=DAMPING_WAIT*1000 && t_now<(DAMPING_WAIT+DAMPING_DURA)*1000)
			rp_GenAmp(RP_CH_2, 1);
		else if (t_now>=(DAMPING_WAIT+DAMPING_DURA)*1000) 
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
	pin_write( TEST_TTL_3, 1);
	
	t_start = micros();
	while((micros()-t_start)<ts_HV*1000)
	{
		t_now = micros()-t_start;
		if(fg_flag){
			t_temp[0] = t_now;
			fg_flag = 0;
		}
		t_temp[1] = t_now - t_temp[0];
		if(t_temp[1] >= UPDATE_RATE)
		{	
			ADC_req(&buff_size, buff, adc_data);
			amp = amp + m1*UPDATE_RATE;
			amp2 = amp2 + m2*UPDATE_RATE;
			rp_GenAmp(RP_CH_1, amp);
			rp_GenAmp(RP_CH_2, amp2);
			t_temp[0]=t_now;
			num++;
		}	
	}
	// printf("num=%d\n",num);
	amp = a2_HV;
	rp_GenAmp(RP_CH_1, amp);
	rp_GenAmp(RP_CH_2, 0);
	pin_write( FGTRIG, 0);
	pin_unexport(FGTRIG);
	pin_unexport(FGTTL);
	pin_unexport(TEST_TTL_0);
	pin_unexport(TEST_TTL_1);
	pin_unexport(TEST_TTL_2);
	pin_unexport(TEST_TTL_3);
	free(t3);
	free(x3);			
	rp_Release();
	write_txt(adc_data, save);
	free(buff);
	
	return 0;
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
	rp_AcqStart();
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
			printf("%f\n",*(adc_data+i));
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
	// if(time<0) time = time*(-1);
//	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
	return time;
}