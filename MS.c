#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
//#include <windows.h>
#include <stdint.h>
#include <unistd.h>
#include "redpitaya/rp.h"
//#include "C:\Users\Quantaser\Dropbox\projects\MassSpectrometer\rp.h"

enum command{
	FUNC_GEN_ADC,
	UART,
	DAC
}com_sel;

long micros(void);
void HVFG(float, float);
void LVFG(float, float);
void ADC_init(void);
void ADC_req(uint32_t*, float*);

//global vars//
/*1. function gen and ADC*/
float freq_HV;
float a0_HV, a1_HV, a2_HV, a_LV;
uint32_t t0_HV, t1_HV, t2_HV;
long t_start, tp;


int main(void)
{
	int com;
	/******function gen******/
	long t_temp[2] = {0,0}, t_now;
	float m1, m2, amp;
	bool fg_flag1=1, fg_flag2=1;
	/******ADC******/
	uint32_t buff_size = 2;
    float *buff = (float *)malloc(buff_size * sizeof(float));
	
	
		do
		{
			printf("Select function : (0):Function Gen and ADC, (1):UART, (2):DAC ");
			scanf("%d",&com);
			fflush(stdin);
		} while(!(com>=0 && com<3));
		
		switch(com)
		{
			case FUNC_GEN_ADC:
				printf("--Selecting Function Gen and ADC---\n");
				printf("set HVFG parameters (freq, t0, a0, t1, a1, t2, a2) :\n");
				scanf("%f%u%f%u%f%u%f", &freq_HV,&t0_HV,&a0_HV,&t1_HV,&a1_HV,&t2_HV,&a2_HV);
				printf("set LVFG parameters (amp) :\n");
				scanf("%f",&a_LV);
				printf("set scan update period (ms): \n");
				scanf("%ld",&tp);
				m1 = (a1_HV - a0_HV)/(t1_HV - t0_HV); //volt/ms
				m2 = (a2_HV - a1_HV)/(t2_HV - t1_HV);
				// printf("m1=%f\n",m1);
				// printf("m2=%f\n",m2);
				amp = a0_HV;
				
				if(rp_Init() != RP_OK){
					fprintf(stderr, "Rp api init failed!\n");
				}
				rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
				rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE);
				rp_GenOutEnable(RP_CH_1);
				rp_GenOutEnable(RP_CH_2);
				ADC_init();
				t_start = micros();
				while((micros()-t_start)<t2_HV*1000)
				{
					// printf("micros= %ld, tstart= %ld\n", micros(),t_start);
					// printf("%ld, %u\n",(micros()-t_start), t2_HV*1000); 
					t_now = micros()-t_start;
					// printf("t_now:%ld\n", t_now);
					if (t_now < t0_HV*1000)
					{
						t_temp[1] = t_now - t_temp[0];
						if(fg_flag1)
						{
							fg_flag1 = 0;
							LVFG(freq_HV, 0); 
						}
						if(t_temp[1] > tp*1000)
						{
							// amp = 0;
							HVFG(freq_HV, 0); 
							ADC_req(&buff_size, buff);
							t_temp[0]=t_now;
						}	
					}
					else if(t_now < t1_HV*1000)
					{		
						t_temp[1] = t_now - t_temp[0];
						if(fg_flag2)
						{
							fg_flag2 = 0;
							LVFG(freq_HV, a_LV);  
						}
						if(t_temp[1] > tp*1000)
						{
							amp = amp + m1*tp;
							HVFG(freq_HV, amp); 
							ADC_req(&buff_size, buff);
							t_temp[0]=t_now;
						}	
					}
					else
					{
//						amp = a1_HV;
						//output fg here//
						t_temp[1] = t_now - t_temp[0];
						if(t_temp[1] > tp*1000)
						{
							// printf("2.t_now:%ld, amp=%f, dt=%ld\n",t_now,amp,t_temp[1]);
							amp = amp + m2*tp;
							HVFG(freq_HV, amp);
							ADC_req(&buff_size, buff);
							t_temp[0]=t_now;
						}
					}					
				}
				HVFG(freq_HV, a2_HV);
				LVFG(freq_HV, 0); 
				free(buff);
				rp_Release();				
			break;
			case BBB:
				printf("now in BBB\n");
				
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
void ADC_req(uint32_t* buff_size, float* buff) {
	rp_AcqGetLatestDataV(RP_CH_1, buff_size, buff);
	printf("%f\n", buff[*buff_size-1]);
}