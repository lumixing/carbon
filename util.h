#include <stdbool.h>

#pragma once

#define defer defer__2(__COUNTER__)
#define defer__2(X) defer__3(X)
#define defer__3(X) defer__4(defer__id##X)
#define defer__4(ID) auto void ID##func(char (*)[]); __attribute__((cleanup(ID##func))) char ID##var[0]; void ID##func(char (*ID##param)[])

#define nfree(ptr) do { if (ptr != NULL) { free(ptr); ptr = NULL; } } while (0)

#define X 0
#define Y 1
#define Z 2

// #define DEBUG

#ifdef DEBUG
#include <windows.h>
#include <psapi.h>
#endif

bool read_entire_file(char **buf, const char *path);
float get_ram_usage_in_mb();