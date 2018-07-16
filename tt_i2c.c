#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>

#include <unistd.h>

#define I2C_ADDR 0x20

int main (void)
{
    int value, v2=0x01;
    int fd, h;

    fd = open("/dev/i2c-0", O_RDWR);
	printf("fd = %d\n", fd);
    // if (fd < 0) {
        // printf("Error opening file: %s\n", strerror(errno));
		// printf("Error opening file: \n");
        // return 1;
    // }
	value=ioctl(fd, I2C_SLAVE, I2C_ADDR);
	printf("value = %d\n", value);
	printf("v2 = %d\n", v2);
	while(1) 
	{
		printf("write=%d\n",write(fd, &v2, 1));	
		sleep(2);
	}
	
    // if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0) {
        // printf("ioctl error: %s\n", strerror(errno));
		// printf("ioctl error: \n");
        // return 1;
    // }

    // for (value=0; value<=255; value++) {
        // if (write(fd, &value, 1) != 1) {
            // printf("Error writing file: %s\n", strerror(errno));
			// printf("Error writing file: \n");
        // }

        // sleep(2);
    // }

    // return 0;
}