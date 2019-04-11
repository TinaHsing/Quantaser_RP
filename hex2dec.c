#include <stdio.h>

char data[8];
int main(int argc, char *argv[])
{
	data = argv[1];
	printf("%s\n", data);
	return 0;
}