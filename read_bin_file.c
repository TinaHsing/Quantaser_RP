#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	FILE *fp;
	size_t cnt = atoi(argv[2]);
	int arr[50000];
	
	fp = fopen(argv[1], "rb");
	fread(arr, sizeof(int), cnt, fp);
	for(int i=0; i<cnt; i++) {
		printf("%d. %d\n", i, arr[i]);
	}
	fclose(fp);
}