#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	FILE *fp;
	size_t cnt = atoi(argv[2]);
	float arr[16384];
	
	fp = fopen(argv[1], "rb");
	fread(arr, sizeof(float), cnt, fp);
	for(int i=0; i<cnt; i++) {
		printf("%d. %f\n", i, arr[i]);
	}
	fclose(fp);
}