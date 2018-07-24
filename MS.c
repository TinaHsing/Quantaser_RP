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
	BBB
}com_sel;

long micros(void);
void HVFG(float, float);

//global vars//
/*1. function gen and ADC*/
float freq_HV;
float a0_HV, a1_HV, a2_HV;
uint32_t t0_HV, t1_HV, t2_HV;
long t_start, tp;


int main(void)
{
	int com;
	long t_temp[2] = {0,0}, t_now;
	float m1, m2, amp;
//	float freq, amp;
//	long t1 = micros(), t2, t3;
	
	
//	Sleep(1000);
//	t2 = micros();
//	Sleep(1000);
//	t3 = micros();
//	printf("%d, %d\n", t2-t1, t3-t2);
//	while(1)
//	{
		
		do
		{
			printf("Select function : (0):Function Gen and ADC, (1):BBB, : ");
			scanf("%d",&com);
			fflush(stdin);
		} while(!(com>=0 && com<3));
		
		switch(com)
		{
			case FUNC_GEN_ADC:
//				long t_temp[2] = {0,0}, t_now;
//				float m1,m2, amp;
				printf("--Selecting Function Gen and ADC---\n");
				printf("set HVFG parameters (freq, t0, a0, t1, a1, t2, a2) :\n");
				scanf("%f%u%f%u%f%u%f", &freq_HV,&t0_HV,&a0_HV,&t1_HV,&a1_HV,&t2_HV,&a2_HV);
				printf("set scan update period (ms): \n");
				scanf("%ld",&tp);
				m1 = (a1_HV - a0_HV)/(t1_HV - t0_HV); //volt/ms
				m2 = (a2_HV - a1_HV)/(t2_HV - t1_HV);
				printf("m1=%f\n",m1);
				printf("m2=%f\n",m2);
				amp = a0_HV;
				t_start = micros();
//				printf("%ld, %u\n",(micros()-t_start), t2_HV*1000); 
				if(rp_Init() != RP_OK){
					fprintf(stderr, "Rp api init failed!\n");
				}
				rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
				rp_GenOutEnable(RP_CH_1);
				while((micros()-t_start)<t2_HV*1000)
				{
//					printf("%ld, %u\n",(micros()-t_start), t2_HV*1000); 
					t_now = micros()-t_start;
					if (t_now < t0_HV*1000){}
					else if(t_now < t1_HV*1000)
					{		
									
						t_temp[1] = micros() - t_temp[0];
						if(t_temp[1] > tp*1000)
						{
							printf("1.t_now:%ld, amp=%f, dt=%ld\n",t_now,amp,t_temp[1]);
							amp = amp + m1*tp;
							HVFG(freq_HV, amp); 
							t_temp[0]=micros();
						}	
					}
					else
					{
//						amp = a1_HV;
						//output fg here//
						t_temp[1] = micros() - t_temp[0];
						if(t_temp[1] > tp*1000)
						{
							printf("2.t_now:%ld, amp=%f, dt=%ld\n",t_now,amp,t_temp[1]);
							amp = amp + m2*tp;
							HVFG(freq_HV, amp);
							t_temp[0]=micros();
						}
					}					
				}
				HVFG(freq_HV, a2_HV);
				rp_Release();				
			break;
			case BBB:
				printf("now in BBB\n");
				
			break;
			default :
				printf("command error, try again!\n");
				
		}
//	}
	
	return 0;
 } 
 
 long micros(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

void HVFG(float freq, float amp){
//	if(rp_Init() != RP_OK){
//		fprintf(stderr, "Rp api init failed!\n");
//	}

	/* Generating frequency */
	rp_GenFreq(RP_CH_1, freq);
//	rp_GenFreq(1, freq);

	/* Generating amplitude */
	rp_GenAmp(RP_CH_1, amp);
//	rp_GenAmp(1, amp);

	/* Generating wave form */
//	rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
//	rp_GenWaveform(1, RP_WAVEFORM_SINE);

	/* Enable channel */
//	rp_GenOutEnable(RP_CH_1);
//	rp_GenOutEnable(1);

	/* Releasing resources */
//	rp_Release();

}
