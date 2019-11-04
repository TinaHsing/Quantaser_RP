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
#include <math.h>
#include "redpitaya/rp.h"


///////*constant define*//////
#define VALUE_MAX 30
#define MAX_PATH 64
#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
// #define CONTINUE
//monitor
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
////////* gpio *///////////
// static int pin_export(int);
// static int pin_unexport(int);
// static int pin_direction(int, int);
// static int pin_write(int, int);

static uint32_t AddrRead(unsigned long);
long micros(void);
// long micros(void);
///////* UART *//////////
static int uart_init(void);
static int release(void);
static int uart_read(int);
static int uart_write();

int uart_num=1;
char *uart_cmd;
unsigned char command[1];

//monitor
void* map_base = (void*)(-1);

#ifdef CONTINUE
long t1, t2;
#endif
int uart_fd = -1;
uint32_t address = 0x40000118;
int main(int argc, char *argv[])
{
	char data[10];
	if(uart_init() < 0)
	{
		printf("Uart init error.\n");
		return -1;
	}
	// printf("addr=%ld\n",atol(argv[1]));
	#ifdef CONTINUE
	t1=micros();
	#endif
	while(1) 
	{
		uart_read(10);
		if(command[0] == '1')
		{
			#ifdef CONTINUE
			while(1)
			{
				t2 = micros();
				if((t2-t1)>100000) {
					sprintf(data,"%d", AddrRead(address));
					uart_write(data);
					t1 = t2;
				}
			}
			#else
				// sprintf(data,"%d", AddrRead(address));
				sprintf(data,"%d", (int)65535);
				uart_write(data);
			#endif
		}
	}
	

	release();
	return 0;
}

static uint32_t AddrRead(unsigned long addr)
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
	// printf("read= %d\n", read_result);
	

	// switch(value)
	// {
		// case START_SCAN:
			// *((unsigned long *) virt_addr) = 0x1;
		// break;
		// case END_SCAN:
			// *((unsigned long *) virt_addr) = 0x2;
		// break;
		// case CLEAR:
			// *((unsigned long *) virt_addr) = 0x0;
		// break;
	// }
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


// static int pin_direction(int pin, int dir){

	// char shell[MAX_PATH];
	// snprintf(shell, MAX_PATH, "echo %s > /sys/class/gpio/gpio%d/direction",((dir==IN)?"in":"out"),pin);
	// system(shell);

	// return 0;
// }

// static int pin_write(int pin, int value)
// {
	// char path[VALUE_MAX];
	// int fd;

	// snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	// get pin value file descrptor
	// fd = open(path, O_WRONLY);
	// if (-1 == fd) {
		// fprintf(stderr, "Unable to to open sysfs pins value file %s for writing\n",path);
		// return -1;
	// }
	// if(value==LOW){
		// write low
		// if (1 != write(fd, "0", 1)) {
			// fprintf(stderr, "Unable to write value\n");
			// return -1;
		// }
	// }
        // else if(value==HIGH){
		// write high
		// if (1 != write(fd, "1", 1)) {
                	// fprintf(stderr, "Unable to write value\n");
                	// return -1;
		// }
	// }else fprintf(stderr, "Nonvalid pin value requested\n");

	// close file
	// close(fd);
	// return 0;
// }
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
			// printf("%s\n", rx_buffer);
			command[0] = rx_buffer[0];
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

static int release(){

    tcflush(uart_fd, TCIFLUSH);
    close(uart_fd);

    return 0;
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