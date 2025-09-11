#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// @free(buf)
bool read_entire_file(char **buf, const char *path) {
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

float get_ram_usage_in_mb() {
#ifdef DEBUG
	PROCESS_MEMORY_COUNTERS pmc;
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		return pmc.WorkingSetSize / 1024.0 / 1024.0;
	}
#endif
	return -1;
}