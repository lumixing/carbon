#include "chunk.h"
#include <cglm/cglm.h>

typedef struct {
	ivec3 size;
	int chunks_len;
	Chunk *chunks;
} World;

int world_lin(ivec3 size, int x, int y, int z);
void world_init(World *world);
void world_free(World *world);
