/*
	this program counts the number of characters in the input file
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	FILE *ifp;
	char file_buffer;
	int i;
	if (argc < 2)
	{
		printf("Error : please include the file name\n");
		return 0;
	}
	ifp = fopen(argv[1], "r");
	i = 0;
	while (fscanf(ifp, "%c", &file_buffer) != EOF)
		i++;
	printf("%d\n", i);
	fclose(ifp);
	return 0;
}