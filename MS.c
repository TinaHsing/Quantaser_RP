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

#include "redpitaya/rp.h"


/* I2C */
#define I2C_ADDR 0x07
/* MOS SW*/
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
#define POUT 968


enum command{
	FUNC_GEN_ADC,
	UART,
	DAC,
	SW
}com_sel;

/*function gen and ADC*/
long micros(void);
void HVFG(float, float);
void LVFG(float, float);
void ADC_init(void);
void ADC_req(uint32_t*, float*);
void write_txt(void);
/* UART */
static int uart_init();
static int release();
static int uart_read(int size);
static int uart_write();
/* I2C */
void i2cOpen();
void i2cClose();
void i2cSetAddress(int);
void WriteRegisterPair(uint8_t, uint16_t);
/* gpio */
static int pin_export(int);
static int pin_unexport(int);
static int pin_direction(int, int);
static int pin_write(int, int);

//global vars//
/*1. function gen and ADC*/
float freq_HV, adc_data[100];
float a0_HV, a1_HV, a2_HV, a_LV;
uint32_t t0_HV, t1_HV, t2_HV;
long t_start, tp;
int idx=0;
/* UART */
int uart_fd = -1;
/* I2C */
int g_i2cFile;
unsigned int i2c_com, i2c_data;

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
	/******UART******/
	char uart_cmd[10];
	char *size = "123456789";
	int uart_return = 0;
	/******DAC******/
	int dac_return = 0;
	/******MOS Switch******/
	int mos_sw;
	
		do
		{
			printf("Select function : (0):Function Gen and ADC, (1):UART, (2):DAC, (3):MOS Switch  ");
			scanf("%d",&com);
			fflush(stdin);
		} while(!(com>=0 && com<4));
		
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
							printf("1. amp=%f\n",amp);
						}	
					}
					else
					{
						t_temp[1] = t_now - t_temp[0];
						printf("temp[1]=%ld\n",t_temp[1]);
						if(t_temp[1] > tp*1000)
						{
							// printf("2.t_now:%ld, amp=%f, dt=%ld\n",t_now,amp,t_temp[1]);
							amp = amp + m2*tp;
							HVFG(freq_HV, amp);
							ADC_req(&buff_size, buff);
							t_temp[0]=t_now;
							printf("2. amp=%f\n",amp);
						}
					}					
				}
				HVFG(freq_HV, a2_HV);
				LVFG(freq_HV, 0);
				free(buff);
				rp_Release();
				write_txt();
				idx = 0;
				
			break;
			case UART:
				printf("--Selecting Function UART---\n");
				if(uart_init() < 0)
				{
					printf("Uart init error.\n");
					return -1;
				}		
				do
				{
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
					printf("Exit uart? Yes:1, No:0\n");
					scanf("%d",&uart_return);
				}while(!uart_return);
				release();
			break;
			case DAC:
				printf("--Selecting Function DAC---\n");
				i2cOpen();
				i2cSetAddress(I2C_ADDR);
				do
				{
					printf("enter DAC command: \n");
					scanf("%x", &i2c_com);
					printf("enter DAC data: \n");
					scanf("%x", &i2c_data);
					WriteRegisterPair(i2c_com, i2c_data);
					printf("Exit DAC setup? Yes:1, No:0\n");
					scanf("%d",&dac_return);
				}
				while(!dac_return);
			break;
			case SW:
				printf("--Selecting Function MOS Switch---\n");
				printf("set switch on(1), off(0)\n");
				scanf("%d", &mos_sw);
				pin_export(POUT);
				pin_direction(POUT, OUT);
				pin_write( POUT, 0);
				if(mos_sw) pin_write( POUT, 1);
				else pin_write( POUT, 0);
				pin_unexport(POUT);
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
void write_txt()
{
	char shell[MAX_PATH];
	system("touch adc_data.txt");
	system("echo "" > adc_data.txt");
	for(int i=0;i<idx;i++)
	{
		sprintf(shell,"echo %f >> adc_data.txt", adc_data[i]);
		system(shell);
	}
}
void ADC_req(uint32_t* buff_size, float* buff) {
	rp_AcqGetLatestDataV(RP_CH_1, buff_size, buff);
	adc_data[idx] = buff[*buff_size-1];
	
	// printf("%f\n", buff[*buff_size-1]);
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
    speed_t baud_rate = B9600;

    /* Baud rate fuctions
    * cfsetospeed - Set output speed
    * cfsetispeed - Set input speed
    * cfsetspeed  - Set both output and input speed */

    cfsetspeed(&settings, baud_rate);

    settings.c_cflag &= ~PARENB; /* no parity */
    settings.c_cflag &= ~CSTOPB; /* 1 stop bit */
    settings.c_cflag &= ~CSIZE;
    settings.c_cflag |= CS8 | CLOCAL; /* 8 bits */
    settings.c_lflag = ICANON; /* canonical mode */
    settings.c_oflag &= ~OPOST; /* raw output */

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
            printf("%i bytes read : %s\n", rx_length, rx_buffer);
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