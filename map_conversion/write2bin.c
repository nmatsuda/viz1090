//
#include<stdio.h> 

extern float mapPoints[3878131];

int main(int argc, char **argv) {
	FILE *file = fopen("mapdata.bin", "wb");
	fwrite(mapPoints, sizeof(mapPoints), 1, file);
	fclose(file);

	return(0);
}
