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
void HVFG(rp_channel_t, float, float);

int main(void)
{
	int com;
//	long t1 = micros(), t2, t3;
	int ch;
	float freq, amp;
	
//	Sleep(1000);
//	t2 = micros();
//	Sleep(1000);
//	t3 = micros();
//	printf("%d, %d\n", t2-t1, t3-t2);
	while(1)
	{
		do
		{
			printf("Select function : (0):Function Gen and ADC, (1):BBB, : ");
			scanf("%d",&com);
			fflush(stdin);
		} while(!(com>=0 && com<3));
		
		switch(com)
		{
			case FUNC_GEN_ADC:
				printf("--Selecting Function Gen and ADC---\n");
				printf("select channel output (0)ch1, (1)ch2: ");
				scanf("%d", &ch);
				printf("set frequency in Hz: ");
				scanf("%f", &freq);
				printf("set amplitude in V: ");
				scanf("%f", &amp);
				HVFG((rp_channel_t)ch, freq, amp);
				
			break;
			case BBB:
				printf("now in BBB\n");
				system("pause");
			break;
			default :
				printf("command error, try again!\n");
				system("pause");
		}
	}
	
	return 0;
 } 
 
 long micros(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

void HVFG(rp_channel_t ch, float freq, float amp){
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
	}

	/* Generating frequency */
	rp_GenFreq(ch, freq);

	/* Generating amplitude */
	rp_GenAmp(ch, amp);

	/* Generating wave form */
	rp_GenWaveform(ch, RP_WAVEFORM_SINE);

	/* Enable channel */
	rp_GenOutEnable(ch);

	/* Releasing resources */
	rp_Release();

}
