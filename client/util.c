#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// @free(buf)
bool read_entire_file(char **buf, char *path) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return false;
	}
	defer { fclose(file); }

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	*buf = malloc(size + 1);
	if (*buf == NULL) {
		return false;
	}
	fread(*buf, size, 1, file);
	(*buf)[size] = 0;

	return true;
}

float Q_rsqrt(float number) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;
	
	x2 = number * 0.5F;
	y = number;
	i = *(long*)&y;           // Evil floating point bit hack
	i = 0x5f3759df - (i >> 1); // What the f***?
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y)); // Newton's method
	return y;
}