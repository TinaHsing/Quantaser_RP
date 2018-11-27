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

/* function gen*/
#define updateRate 30 //us
#define FN_GEN_MODE 0
#define CHIRP_MODE 1

#define TTL_WAIT 1
#define TTL_DURA 500
#define DAMPING_WAIT 1
#define DAMPING_DURA 50
#define CHIRP_WAIT 1
#define SCAN_WAIT 10
#define CHIRP_SWEEP_TIME 2

#define FGTRIG 977
#define FGTTL 978 
#define TEST_TTL_1 979
#define TEST_TTL_2 980
#define TEST_TTL_3 981


// /* I2C */
// #define I2C_ADDR 0x07
/* MOS SW*/
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
#define POUT1 968 //DIO0_P
#define POUT2 969
#define POUT3 970
#define POUT4 971
#define UART1 972
#define UART2 973
#define UART3 974
#define UART4 975 //DIO7_P

#define M_PI 3.14159265358979323846

/* DAC LTC2615 */
#define DAC0_ADD 0x10
#define DAC1_ADD 0x52
#define CC 0b0011
#define ref 2.5
#define max 16383

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

#define FN_GAIN 3

//global vars//
/*1. function gen and ADC*/
float freq_HV;

float a0_HV, a1_HV, a2_HV, a_LV;
#if FN_GEN_MODE
uint32_t t0_HV, t1_HV, t2_HV;
#else
uint32_t ts_HV;
#endif

long t_start;
int idx=0;
/* UART */
int uart_fd = -1;
/* I2C */
int g_i2cFile;
unsigned int dac_num;
float dac_value;

enum command{
	FUNC_GEN_ADC,
	CHIRP,
	UART,
	DAC,
	SW,
	TEST
}com_sel;

/*function gen and ADC*/
long micros(void);
void HVFG(float, float);
void LVFG(float, float);
void ADC_init(void);
void ADC_req(uint32_t*, float*, float*);
void write_txt(float*, int);
/* UART */
static int uart_init(void);
static int release(void);
static int uart_read(int);
static int uart_write();
void connect_uart(int *);
void disconnect_uart(void);
/* I2C */
void i2cOpen(void);
void i2cClose(void);
void i2cSetAddress(int);
void WriteRegisterPair(uint8_t, uint16_t);
void LTC2615_write(bool, uint8_t, float);
void DAC_out(uint8_t, float);
void DAC_out_init(void);
/* gpio */
static int pin_export(int);
static int pin_unexport(int);
static int pin_direction(int, int);
static int pin_write(int, int);

int main(void)
{
	int com;
	/******function gen******/
	long arb_size = 16384, t_now;
	#if CHIRP_MODE
	long t0;
	#endif
	#if FN_GEN_MODE
	float *buff = (float *)malloc(buff_size * sizeof(float));
	int	data_size=0, save=0;
	/******ADC******/
	float *adc_data, m2;
	uint32_t buff_size = 2;
	#endif
	long t_temp[2] = {0,0};
	float start_freq, final_freq, k, freq_factor;
	int sweep_time;
	float m1, m2, amp, amp2;
	bool fg_flag=1;
	
	
    
	/******UART******/
	char uart_cmd[15];
	int uart_num=1;
	char *size = "123456789123456789123456789123456789";
	int uart_return = 0;
	/******DAC******/
	int dac_return = 0;
	/******MOS Switch******/
	int mos_sw1, mos_sw2, mos_sw3, mos_sw4;
	printf("version v2.0\n");//date 2018-11-26
	// system("cat /opt/redpitaya/fpga/classic/fpga.bit > /dev/xdevcfg");
	system("cat /opt/redpitaya/fpga/red_pitaya_top.bit > /dev/xdevcfg");
	DAC_out_init();
		do
		{
			printf("Select function : (0):Function Gen, (1):CHIRP, (2):UART, (3):DAC, (4):MOS Switch  ");
			scanf("%d",&com);
			fflush(stdin);
		} while(!(com>=FUNC_GEN_ADC && com<=TEST));
		
		switch(com)
		{
			#if FN_GEN_MODE //ch1 plus ch2
			case FUNC_GEN_ADC:
				printf("--Selecting Function Gen and ADC---\n");
				printf("set HVFG parameters (freq, t0, a0, t1, a1, t2, a2) :\n");
				scanf("%f%u%f%u%f%u%f", &freq_HV,&t0_HV,&a0_HV,&t1_HV,&a1_HV,&t2_HV,&a2_HV);
				
				
				
				printf("save data to .txt file? yes(1), no(0) : \n");
				scanf("%d",&save);
				
				
				
				
				a0_HV /= 10;
				a1_HV /= 10;
				a2_HV /= 10;
				
				pin_export(FGTRIG);
				pin_direction(FGTRIG, OUT);
				pin_write( FGTRIG, 0);
				data_size = t2_HV*1000/updateRate;
				adc_data = (float *) malloc(sizeof(float)*data_size);
				m1 = (a1_HV - a0_HV)/(t1_HV - t0_HV)/1000; //volt/us
				m2 = (a2_HV - a1_HV)/(t2_HV - t1_HV)/1000;
				amp = a0_HV;
				
				if(rp_Init() != RP_OK){
					fprintf(stderr, "Rp api init failed!\n");
				}
				rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
				LVFG(freq_HV, 0); 
				HVFG(freq_HV, 0); 
				rp_GenOutEnable(RP_CH_1);
				rp_GenOutEnable(RP_CH_2);
				
				ADC_init();
				pin_write( FGTRIG, 1);
				t_start = micros();
				
				while((micros()-t_start)<t2_HV*1000)
				{
					t_now = micros()-t_start;
					if (t_now < t0_HV*1000)
					{
						t_temp[1] = t_now - t_temp[0];
						// if(fg_flag1)
						// {
							// fg_flag1 = 0;
							// LVFG(freq_HV, 0); 
						// }
						if(t_temp[1] >= updateRate)
						{
							// HVFG(freq_HV, 0); 
							ADC_req(&buff_size, buff, adc_data);
							t_temp[0]=t_now;
						}	
						
					}
					else if(t_now < t1_HV*1000)
					{		
						t_temp[1] = t_now - t_temp[0];
						if(fg_flag)
						{
							fg_flag = 0;
							// LVFG(freq_HV, a_LV);  
							rp_GenAmp(RP_CH_2, a_LV);
						}
						if(t_temp[1] >= updateRate)
						{	
							// num++;
							amp = amp + m1*updateRate;
							// HVFG(freq_HV, amp); 
							rp_GenAmp(RP_CH_1, amp);
							ADC_req(&buff_size, buff, adc_data);
							t_temp[0]=t_now;
						}	
						
					}
					else
					{
						
						t_temp[1] = t_now - t_temp[0];
						if(t_temp[1] >= updateRate)
						{
							// num2++;
							amp = amp + m2*updateRate;
							// HVFG(freq_HV, amp);
							rp_GenAmp(RP_CH_1, amp);
							ADC_req(&buff_size, buff, adc_data);
							t_temp[0]=t_now;
							// printf("2. amp=%f\n",amp);
						}
						
					}					
				}
				rp_GenAmp(RP_CH_1, a2_HV);
				rp_GenAmp(RP_CH_2, 0);
				pin_write( FGTRIG, 0);
				pin_unexport(FGTRIG);
				free(buff);
				rp_Release();
				write_txt(adc_data, save);
				free(adc_data);
				idx = 0;
				
			break;
			#else 
			case FUNC_GEN_ADC: //ch1_start_flag, ttl_start_flag, ttl_end_flag, chirp_flag;
				if(rp_Init() != RP_OK){
							fprintf(stderr, "Rp api init failed!\n");
						}
				pin_export(FGTRIG);
				pin_export(FGTTL);
				pin_export(TEST_TTL_1);
				pin_export(TEST_TTL_2);
				pin_export(TEST_TTL_3);
				pin_direction(FGTRIG, OUT);
				pin_direction(FGTTL, OUT);
				pin_direction(TEST_TTL_1, OUT);
				pin_direction(TEST_TTL_2, OUT);
				pin_direction(TEST_TTL_3, OUT);
				pin_write( FGTRIG, 0);
				pin_write( FGTTL, 0);
				pin_write( TEST_TTL_1, 0);
				pin_write( TEST_TTL_2, 0);
				pin_write( TEST_TTL_3, 0);
				printf("set HVFG parameters (freq_HV(Hz), ts_HV(ms), a0_HV, a1_HV, a2_HV(Volt, 0~1V)) :\n");
				scanf("%f%u%f%f%f", &freq_HV,&ts_HV,&a0_HV,&a1_HV, &a2_HV);
				printf("set chirping amplitude (0~10V) :\n");
				scanf("%f",&a_LV);
				printf("enter freq factor and chirp final freq in KHz: ");
				scanf("%f%f", &freq_factor, &final_freq);
				while ( getchar() != '\n' );
				rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC);
				rp_GenFreq(RP_CH_1, freq_HV);
				rp_GenAmp(RP_CH_1, 0);
				rp_GenAmp(RP_CH_2, 0);
				rp_GenOutEnable(RP_CH_1);
				rp_GenOutEnable(RP_CH_2);
				
				
				start_freq = 0.5*freq_HV/1000;
				// printf("enter sweep time in ms: ");
				// scanf("%d",&sweep_time);
				sweep_time = CHIRP_SWEEP_TIME;
				amp2 = 0;
				a_LV /= 10;
				float *t2 = (float *)malloc(arb_size * sizeof(float));
				float *x2 = (float *)malloc(arb_size * sizeof(float));
				k = (final_freq - start_freq) / sweep_time;
				for(long i = 0; i < arb_size; i++){
					t2[i] = (float)sweep_time / arb_size * i;
					x2[i] = sin(2*M_PI*(start_freq*t2[i] + 0.5*k*t2[i]*t2[i]));
				}
				rp_GenArbWaveform(RP_CH_2, x2, arb_size);
				
				
				m1 = (a1_HV - a0_HV)/(ts_HV)/1000; //volt/us
				m2 = 1/(ts_HV)/1000;
				amp = a0_HV;
				rp_GenAmp(RP_CH_1, amp);
				
				t_start = micros();
				while((micros()-t_start)<TTL_WAIT*1000){};
				pin_write( FGTTL, 1);
				pin_write( TEST_TTL_1, 1);
				
				t_start = micros();
				while((micros()-t_start)<TTL_DURA*1000){
					t_now = micros()-t_start;
					if(t_now>=DAMPING_WAIT*1000 && t_now<(DAMPING_WAIT+DAMPING_DURA)*1000)
						rp_GenAmp(RP_CH_2, 1);
					else if (t_now>=(DAMPING_WAIT+DAMPING_DURA)*1000) 
						rp_GenAmp(RP_CH_2, 0);
				}
				pin_write( FGTTL, 0);
				pin_write( TEST_TTL_2, 1);
				
				// t_start = micros();
				// while((micros()-t_start)<CHIRP_WAIT*1000){};
				/*add chirp below*/
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
				rp_GenFreq(RP_CH_2, 1000.0/sweep_time);
				rp_GenAmp(RP_CH_2, a_LV);
				t_start = micros();		
				while((micros()-t_start)<sweep_time*1000){};
				rp_GenAmp(RP_CH_2, 0);
				
				t_start = micros();	
				rp_GenFreq(RP_CH_2, freq_factor*freq_HV);	
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE);				
				while((micros()-t_start)<SCAN_WAIT*1000){};
				
				pin_write( FGTRIG, 1);
				t_start = micros();
				while((micros()-t_start)<ts_HV*1000)
				{
					t_now = micros()-t_start;
					if(fg_flag){
						t_temp[0] = t_now;
						fg_flag = 0;
					}
					t_temp[1] = t_now - t_temp[0];
					if(t_temp[1] >= updateRate)
					{	
						amp = amp + m1*updateRate;
						amp2 = amp2 + m2*updateRate;
						rp_GenAmp(RP_CH_1, amp);
						rp_GenAmp(RP_CH_2, amp2);
						t_temp[0]=t_now;
					}	
				}
				amp = a2_HV;
				rp_GenAmp(RP_CH_1, amp);
				rp_GenAmp(RP_CH_2, 0);
				pin_write( FGTRIG, 0);
				pin_unexport(FGTRIG);
				free(t2);
				free(x2);
				rp_Release();
			break;
			#endif
			case TEST:
				if(rp_Init() != RP_OK){
						fprintf(stderr, "Rp api init failed!\n");
					}
				pin_export(FGTRIG);
				pin_export(FGTTL);
				pin_export(TEST_TTL_1);
				pin_export(TEST_TTL_2);
				pin_export(TEST_TTL_3);
				pin_direction(FGTRIG, OUT);
				pin_direction(FGTTL, OUT);
				pin_direction(TEST_TTL_1, OUT);
				pin_direction(TEST_TTL_2, OUT);
				pin_direction(TEST_TTL_3, OUT);
				pin_write( FGTRIG, 0);
				pin_write( FGTTL, 0);
				pin_write( TEST_TTL_1, 0);
				pin_write( TEST_TTL_2, 0);
				pin_write( TEST_TTL_3, 0);
				printf("set HVFG parameters (freq_HV(Hz), ts_HV(ms), a0_HV, a1_HV, a2_HV(Volt, 0~1V)) :\n");
				scanf("%f%u%f%f%f", &freq_HV,&ts_HV,&a0_HV,&a1_HV, &a2_HV);
				printf("set chirping amplitude (0~10V) :\n");
				scanf("%f",&a_LV);
				printf("enter freq factor and chirp final freq in KHz: ");
				scanf("%f%f", &freq_factor, &final_freq);
				start_freq = 0.5*freq_HV/1000;
				// printf("set chirping amplitude (0~10V) :\n");
				// scanf("%f",&a_LV);
				// printf("enter chirp start and final freq in KHz: ");
				// scanf("%f%f",&start_freq, &final_freq);
				// printf("enter sweep time in ms: ");
				// scanf("%d",&sweep_time);
				while ( getchar() != '\n' );
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
				m2 = 1/(ts_HV)/1000;
				amp = a0_HV;
				rp_GenAmp(RP_CH_1, amp);
				
				t_start = micros();
				while((micros()-t_start)<TTL_WAIT*1000){};
				pin_write( FGTTL, 1);
				pin_write( TEST_TTL_1, 1);
				
				t_start = micros();
				while((micros()-t_start)<TTL_DURA*1000){
					t_now = micros()-t_start;
					if(t_now>=DAMPING_WAIT*1000 && t_now<(DAMPING_WAIT+DAMPING_DURA)*1000)
						rp_GenAmp(RP_CH_2, 1);
					else if (t_now>=(DAMPING_WAIT+DAMPING_DURA)*1000) 
						rp_GenAmp(RP_CH_2, 0);
				}
				pin_write( FGTTL, 0);
				
				t_start = micros();
				while((micros()-t_start)<TTL_DURA*1000){
					t_now = micros()-t_start;
					if(t_now>=DAMPING_WAIT*1000 && t_now<(DAMPING_WAIT+DAMPING_DURA)*1000)
						rp_GenAmp(RP_CH_2, 1);
					else if (t_now>=(DAMPING_WAIT+DAMPING_DURA)*1000) 
						rp_GenAmp(RP_CH_2, 0);
				}
				
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
				rp_GenFreq(RP_CH_2, 1000.0/sweep_time);
				rp_GenAmp(RP_CH_2, a_LV);
				pin_write( TEST_TTL_2, 1);
				t0 = micros();		
				
				while((micros()-t0)<sweep_time*1000){
					// printf("dt=%ld\n",micros()-t0);
				}
				rp_GenOutDisable(RP_CH_2);
				free(t3);
				free(x3);
				rp_Release();
			break;
			#if CHIRP_MODE
			case CHIRP:
				if(rp_Init() != RP_OK){
						fprintf(stderr, "Rp api init failed!\n");
					}
				
				pin_export(TEST_TTL_2);				
				pin_direction(TEST_TTL_2, OUT);
				pin_write( TEST_TTL_2, 0);
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
				rp_GenAmp(RP_CH_2, 0);
				rp_GenOutEnable(RP_CH_2);
				printf("set chirping amplitude (0~10V) :\n");
				scanf("%f",&a_LV);
				printf("enter chirp start and final freq in KHz: ");
				scanf("%f%f",&start_freq, &final_freq);
				printf("enter sweep time in ms: ");
				scanf("%d",&sweep_time);
				a_LV /= 10;
				float *t = (float *)malloc(arb_size * sizeof(float));
				float *x = (float *)malloc(arb_size * sizeof(float));
				k = (final_freq - start_freq) / sweep_time;
				for(long i = 0; i < arb_size; i++){
					t[i] = (float)sweep_time / arb_size * i;
					x[i] = sin(2*M_PI*(start_freq*t[i] + 0.5*k*t[i]*t[i]));
				}
				rp_GenArbWaveform(RP_CH_2, x, arb_size);
				
				rp_GenFreq(RP_CH_2, 1000.0/sweep_time);
				// sleep(1);
				rp_GenAmp(RP_CH_2, a_LV);
				pin_write( TEST_TTL_2, 1);
				t0 = micros();		
				
				while((micros()-t0)<sweep_time*1000){
					// printf("dt=%ld\n",micros()-t0);
				}
				rp_GenOutDisable(RP_CH_2);
				free(t);
				free(x);
				rp_Release();
			break;
			#else //test burst mode
			case CHIRP:
				if(rp_Init() != RP_OK){
						fprintf(stderr, "Rp api init failed!\n");
					}
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_ARBITRARY);
				rp_GenAmp(RP_CH_2, 0);
				rp_GenMode(RP_CH_2, RP_GEN_MODE_BURST);
				
				printf("set chirping amplitude (0~10V) :\n");
				scanf("%f",&a_LV);
				printf("enter chirp start and final freq in KHz: ");
				scanf("%f%f",&start_freq, &final_freq);
				printf("enter sweep time in ms: ");
				scanf("%d",&sweep_time);
				a_LV /= 10;
				float *t = (float *)malloc(arb_size * sizeof(float));
				float *x = (float *)malloc(arb_size * sizeof(float));
				k = (final_freq - start_freq) / sweep_time;
				for(long i = 0; i < arb_size; i++){
					t[i] = (float)sweep_time / arb_size * i;
					x[i] = sin(2*M_PI*(start_freq*t[i] + 0.5*k*t[i]*t[i]));
				}
				rp_GenArbWaveform(RP_CH_2, x, arb_size);
				
				rp_GenFreq(RP_CH_2, 1000.0/sweep_time);
				// sleep(1);
				rp_GenAmp(RP_CH_2, a_LV);
				// t0 = micros();	
				rp_GenBurstCount(RP_CH_2, 1);
				rp_GenBurstRepetitions(RP_CH_2, 1);
				rp_GenBurstPeriod(RP_CH_2, 1000);
				rp_GenTrigger(1);				
				rp_GenOutEnable(RP_CH_2);
				// while((micros()-t0)<sweep_time*1000){
					
				// }
				// rp_GenOutDisable(RP_CH_2);
				free(t);
				free(x);
				rp_Release();
			break;	
			#endif
			case UART:
			    printf("\n");
				printf("--Selecting Function UART---\n");
				// if(uart_init() < 0)
				// {
					// printf("Uart init error.\n");
					// return -1;
				// }	
				pin_export(UART1);
				pin_export(UART2);
				pin_export(UART3);
				pin_export(UART4);
				pin_direction(UART1, OUT);
				pin_direction(UART2, OUT);
				pin_direction(UART3, OUT);
				pin_direction(UART4, OUT);
				do
				{
					if(uart_init() < 0)
					{
						printf("Uart init error.\n");
						return -1;
					}
					printf("enter UART number to communicate (1~4): \n");
					scanf("%d",&uart_num);
					connect_uart(&uart_num);
					// uart_write("*CLS");
					// uart_write("SYST:REM");
					printf("enter command: \n");
					scanf("%s", uart_cmd);
					if(uart_write(uart_cmd) < 0){
					printf("Uart write error\n");
					return -1;
					}
		
					if(uart_read(strlen(size)) < 0)
					{
						printf("Uart read error\n");
						return -1;
					}
					release();
					printf("Exit uart? Yes:1, No:0\n");
					scanf("%d",&uart_return);
				}while(!uart_return);
				disconnect_uart();
				pin_unexport(UART1);
				pin_unexport(UART2);
				pin_unexport(UART3);
				pin_unexport(UART4);
				// release();
			break;
			case DAC:
				printf("--Selecting Function DAC---\n");
				i2cOpen();
				do
				{
					printf("Select DAC#(1~10): ");
					scanf("%d",&dac_num);
					
					printf("Enter DAC value to output(0~10): ");
					scanf("%f",&dac_value);
					
					DAC_out((uint8_t)dac_num, dac_value);
					
					printf("Exit DAC setup? Yes:1, No:0\n");
					scanf("%d",&dac_return);
				}
				while(!dac_return);
				i2cClose();
			break;
			case SW:
				printf("--Selecting Function MOS Switch---\n");
				printf("set switch status : on(1), off(0)\n");
				printf("SW1 SW2 SW3 SW4 (ex: 1 0 0 1)");
				scanf("%d%d%d%d", &mos_sw1, &mos_sw2, &mos_sw3, &mos_sw4);
				pin_export(POUT1);
				pin_export(POUT2);
				pin_export(POUT3);
				pin_export(POUT4);
				pin_direction(POUT1, OUT);
				pin_direction(POUT2, OUT);
				pin_direction(POUT3, OUT);
				pin_direction(POUT4, OUT);
				// pin_write( POUT1, 0);
				// pin_write( POUT2, 0);
				// pin_write( POUT3, 0);
				// pin_write( POUT4, 0);
				if(mos_sw1) pin_write( POUT1, 1);
				else pin_write( POUT1, 0);
				if(mos_sw2) pin_write( POUT2, 1);
				else pin_write( POUT2, 0);
				if(mos_sw3) pin_write( POUT3, 1);
				else pin_write( POUT3, 0);
				if(mos_sw4) pin_write( POUT4, 1);
				else pin_write( POUT4, 0);
				pin_unexport(POUT1);
				pin_unexport(POUT2);
				pin_unexport(POUT3);
				pin_unexport(POUT4);
			break;
			default :
				printf("command error, try again!\n");
		}
	
	return 0;
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
void HVFG(float freq, float amp){
	rp_GenFreq(RP_CH_1, freq);
	rp_GenAmp(RP_CH_1, amp);
}
void LVFG(float freq, float amp) {
	rp_GenFreq(RP_CH_2, freq);
	rp_GenAmp(RP_CH_2, amp);
}
void ADC_init(void){
	rp_AcqReset();
	rp_AcqSetDecimation(1);
	rp_AcqStart();
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

static int uart_init(){

    uart_fd = open("/dev/ttyPS1", O_RDWR | O_NOCTTY | O_NDELAY);

    if(uart_fd == -1){
        fprintf(stderr, "Failed to open uart.\n");
        return -1;
    }

    struct termios settings;
    tcgetattr(uart_fd, &settings);

    /*  CONFIGURE THE UART
    *  The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
    *       Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
    *       CSIZE:- CS5, CS6, CS7, CS8
    *       CLOCAL - Ignore modem status lines
    *       CREAD - Enable receiver
    *       IGNPAR = Ignore characters with parity errors
    *       ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
    *       PARENB - Parity enable
    *       PARODD - Odd parity (else even) */

    /* Set baud rate - default set to 9600Hz */
	cfsetispeed(&settings,B9600);
	cfsetospeed(&settings,B9600);

	settings.c_cflag &= ~PARENB;          /* Disables the Parity   Enable bit(PARENB),So No Parity   */
	settings.c_cflag &= ~CSTOPB;          /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
	settings.c_cflag &= ~CSIZE;           /* Clears the mask for setting the data size             */
	settings.c_cflag |=  CS8;             /* Set the data bits = 8                                 */

	settings.c_cflag &= ~CRTSCTS;         /* No Hardware flow Control                         */
	settings.c_cflag |= CREAD | CLOCAL;   /* Enable receiver,Ignore Modem Control lines       */ 


	settings.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
	settings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Non Cannonical mode                            */

	settings.c_oflag &= ~OPOST;/*No Output Processing*/

	/* Setting Time outs */
	settings.c_cc[VMIN] = 20; /* Read at least 10 characters */
	settings.c_cc[VTIME] = 0; /* Wait indefinetly   */
	
	settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    settings.c_oflag &= ~OPOST;
    settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    /* Setting attributes */
    tcflush(uart_fd, TCIFLUSH);
    tcsetattr(uart_fd, TCSANOW, &settings);

    return 0;
}
static int uart_read(int size){

    /* Read some sample data from RX UART */

    /* Don't block serial read */
    fcntl(uart_fd, F_SETFL, FNDELAY);
    while(1){
        if(uart_fd == -1){
            fprintf(stderr, "Failed to read from UART.\n");
            return -1;
        }
        unsigned char rx_buffer[size];

        int rx_length = read(uart_fd, (void*)rx_buffer, size);
		// printf("length = %d\n", rx_length);
        if (rx_length < 0){

            /* No data yet avaliable, check again */
            if(errno == EAGAIN){
                // fprintf(stderr, "AGAIN!\n");
                continue;
            /* Error differs */
            }else{
                fprintf(stderr, "Error!\n");
                return -1;
            }

        }else if (rx_length == 0){
            fprintf(stderr, "No data waiting\n");
        /* Print data and exit while loop */
        }else{
            rx_buffer[rx_length] = '\0';
            // printf("%i bytes read : %s\n", rx_length, rx_buffer);
			printf("%s\n", rx_buffer);
            break;
        }
    }

    return 0;
}
static int uart_write(char *data){

    /* Write some sample data into UART */
    /* ----- TX BYTES ----- */
    int msg_len = strlen(data);

    int count = 0;
    char tx_buffer[msg_len+1];
	// char tx_buffer[msg_len];

    strncpy(tx_buffer, data, msg_len);
    tx_buffer[msg_len++] = 0x0a; //New line numerical value

    if(uart_fd != -1){
        count = write(uart_fd, &tx_buffer, (msg_len));
    }
    if(count < 0){
        fprintf(stderr, "UART TX error.\n");
        return -1;
    }

    return 0;
}
void connect_uart(int *num) {
	switch(*num)
	{
		case 1:
			pin_write( UART1, 1);
			pin_write( UART2, 0);
			pin_write( UART3, 0);
			pin_write( UART4, 0);
		break;
		case 2:
			pin_write( UART1, 0);
			pin_write( UART2, 1);
			pin_write( UART3, 0);
			pin_write( UART4, 0);
		break;
		case 3:
			pin_write( UART1, 0);
			pin_write( UART2, 0);
			pin_write( UART3, 1);
			pin_write( UART4, 0);
		break;
		case 4:
			pin_write( UART1, 0);
			pin_write( UART2, 0);
			pin_write( UART3, 0);
			pin_write( UART4, 1);
		break;
		default:
			pin_write( UART1, 0);
			pin_write( UART2, 0);
			pin_write( UART3, 0);
			pin_write( UART4, 0);
	}
}
void disconnect_uart(void) {
	pin_write( UART1, 0);
	pin_write( UART2, 0);
	pin_write( UART3, 0);
	pin_write( UART4, 0);
}
static int release(){

    tcflush(uart_fd, TCIFLUSH);
    close(uart_fd);

    return 0;
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

void LTC2615_write(bool sel, uint8_t ch, float value)
{
	uint8_t t[2];
	uint16_t code;
	
	code = (uint16_t)(value/ref*max);
	t[0] = (code >> 8)<<2 | ((uint8_t)code & 0b11000000)>>6;
	t[1] = (uint8_t)code << 2;
	
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

void DAC_out_init()
{
	i2cOpen();
	DAC_out(DAC1, 0);
	DAC_out(DAC2, 0);
	DAC_out(DAC3, 0);
	DAC_out(DAC4, 0);
	DAC_out(DAC5, 0);
	DAC_out(DAC6, 0);
	DAC_out(DAC7, 0);
	DAC_out(DAC8, 0);
	DAC_out(DAC9, 0);
	DAC_out(DAC10, 0);
	i2cClose();
};
