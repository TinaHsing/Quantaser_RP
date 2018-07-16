#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>

#include <unistd.h>

#define I2C_ADDR 0x20

int main (void)
{
    int value;
    int fd;

    fd = open("/dev/i2c-0", O_RDWR);

    if (fd < 0) {
        // printf("Error opening file: %s\n", strerror(errno));
		printf("Error opening file: \n");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0) {
        // printf("ioctl error: %s\n", strerror(errno));
		printf("ioctl error: \n");
        return 1;
    }

    for (value=0; value<=255; value++) {
        if (write(fd, &value, 1) != 1) {
            // printf("Error writing file: %s\n", strerror(errno));
			printf("Error writing file: \n");
        }

        sleep(2);
    }

    return 0;
}