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

/* I2C */
void i2cOpen(void);
void i2cClose(void);
void i2cSetAddress(int);
void WriteRegisterPair(uint8_t, uint16_t);
void LTC2615_write(bool, uint8_t, float);
void DAC_out(uint8_t, float);
void DAC_out_init(void);

int g_i2cFile;
int dac_num;
float dac_value;

int main(int argc, char *argv[])
{
	i2cOpen();
	// printf("Select DAC#(1~10): ");
	// scanf("%d",&dac_num);
	
	// printf("Enter DAC value to output(0~10): ");
	// scanf("%f",&dac_value);
	
	dac_num = atoi(argv[1]);
	dac_value = atof(argv[2]);
	
	DAC_out((uint8_t)dac_num, dac_value);
	
	i2cClose();
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

void LTC2615_write(bool sel, uint8_t ch, float value)
{
	#ifdef DAC_BIT_14
		uint8_t t[2];
	#endif
	uint16_t code;
	
	code = (uint16_t)(value/ref*max);
	#ifdef DAC_BIT_14
		t[0] = (code >> 8)<<2 | ((uint8_t)code & 0b11000000)>>6;
		t[1] = (uint8_t)code << 2;
	#endif
	
	if(!sel)
	{
		i2cSetAddress(DAC0_ADD);
		#ifdef DAC_BIT_14
			WriteRegisterPair((CC << 4) | ch, (uint16_t)t[1]<<8 | t[0]);
		#endif
		#ifdef DAC_BIT_16
			WriteRegisterPair((CC << 4) | ch, code >> 2);
		#endif
	}
	else
	{
		i2cSetAddress(DAC1_ADD);
		#ifdef DAC_BIT_14
			WriteRegisterPair((CC << 4) | ch, (uint16_t)t[1]<<8 | t[0]);
		#endif
		#ifdef DAC_BIT_16
			WriteRegisterPair((CC << 4) | ch, code >> 2);
		#endif
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