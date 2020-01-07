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

///////*Uart SW pin define*////////
#define UART1 972
#define UART2 973
#define UART3 974
#define UART4 975 

///////*constant define*//////
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

///////* UART *//////////
static int uart_init(void);
static int release(void);
static int uart_read(int);
static int uart_write();
void connect_uart(int *);
void disconnect_uart(void);

int uart_num=1;
char *uart_cmd;



int uart_fd = -1;
int main(int argc, char *argv[])
{
	char *size = "123456789123456789123456789123456789";
	
	pin_export(UART1);
	pin_export(UART2);
	pin_export(UART3);
	pin_export(UART4);
	pin_direction(UART1, OUT);
	pin_direction(UART2, OUT);
	pin_direction(UART3, OUT);
	pin_direction(UART4, OUT);
	
	if(uart_init() < 0)
	{
		printf("Uart init error.\n");
		return -1;
	}
	// printf("enter UART number to communicate (1~4): \n");
	// scanf("%d",&uart_num);
	
	// printf("enter command: \n");
	// scanf("%s", uart_cmd);
	
	uart_num = atoi(argv[1]);
	uart_cmd = argv[2];
	// printf("uart_cmd = %s\n",uart_cmd);
	connect_uart(&uart_num);
	// if(uart_write(uart_cmd) < 0){
	if(uart_write("@254BR?;FF") < 0){
	printf("Uart write error\n");
	return -1;
	}

	if(uart_read(strlen(size)) < 0)
	{
		printf("Uart read error\n");
		return -1;
	}
	release();
	
	disconnect_uart();
	pin_unexport(UART1);
	pin_unexport(UART2);
	pin_unexport(UART3);
	pin_unexport(UART4);
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