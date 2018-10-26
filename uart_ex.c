
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>                 //Used for UART
#include <fcntl.h>                  //Used for UART
#include <termios.h>                //Used for UART
#include <errno.h>
// #include <windows.h>


/* Inline function definition */
static int uart_init();
static int release();
// static int uart_read(int size);
static int uart_read();
static int uart_write();

/* File descriptor definition */
int uart_fd = -1;

static int uart_init(){

    // uart_fd = open("/dev/ttyPS1", O_RDWR | O_NOCTTY | O_NDELAY);
	uart_fd = open("/dev/ttyPS1", O_RDWR | O_NOCTTY | O_SYNC);

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
    // speed_t baud_rate = B9600;

    /* Baud rate fuctions
    * cfsetospeed - Set output speed
    * cfsetispeed - Set input speed
    * cfsetspeed  - Set both output and input speed */

    // cfsetspeed(&settings, baud_rate);
	cfsetispeed(&settings,B9600);
	cfsetospeed(&settings,B9600);

    // settings.c_cflag &= ~PARENB; /* no parity */
    // settings.c_cflag &= ~CSTOPB; /* 1 stop bit */
    // settings.c_cflag &= ~CSIZE;
	// settings.c_cflag &= ~ICRNL;
    // settings.c_cflag |= CS8 | CLOCAL; /* 8 bits */
    // settings.c_lflag = ICANON; /* canonical mode */
    // settings.c_oflag &= ~OPOST; /* raw output */
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
    // termios_p->c_cflag &= ~(CSIZE | PARENB);
    // termios_p->c_cflag |= CS8;

    /* Setting attributes */
    tcflush(uart_fd, TCIFLUSH);
    tcsetattr(uart_fd, TCSANOW, &settings);

    return 0;
}

// static int uart_read(int size){
static int uart_read(){

    /* Read some sample data from RX UART */

    /* Don't block serial read */
    fcntl(uart_fd, F_SETFL, FNDELAY);

    while(1){
        if(uart_fd == -1){
            fprintf(stderr, "Failed to read from UART.\n");
            return -1;
        }

        // unsigned char rx_buffer[size];
		unsigned char rx_buffer[255];

        // int rx_length = read(uart_fd, (void*)rx_buffer, size);
		int rx_length = read(uart_fd, (void*)rx_buffer, 20);
		printf("len= %d\n",rx_length);
        if (rx_length < 0){

            /* No data yet avaliable, check again */
            if(errno == EAGAIN){
                fprintf(stderr, "AGAIN!\n");
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

int main(int argc, char *argv[]){

    char *data = "@254BR?;FF";
	// char *data2 = "123456789abcde";

    /* uart init */
    if(uart_init() < 0){
        printf("Uart init error.\n");
        return -1;
    }

    /* Sample write */
    if(uart_write(data) < 0){
        printf("Uart write error\n");
        return -1;
    }
	// sleep(1);
    /* Sample read */
    // if(uart_read(strlen(data2)) < 0){
	if(uart_read() < 0){
        printf("Uart read error\n");
        return -1;
    }

    /* CLOSING UART */
    release();

    return 0;
}