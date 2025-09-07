#include "world.h"

#include "../util.h"

int world_lin(ivec3 size, int x, int y, int z) {
	return x + y * size[X] + z * size[X] * size[Y];
}

void world_init(World *world) {
	world->chunks_len = world->size[X] * world->size[Y] * world->size[Z];
	world->chunks = malloc(world->chunks_len * sizeof(Chunk));
	assert(world->chunks != NULL);

	for (int x = 0; x < world->size[X]; x++) {
		for (int y = 0; y < world->size[Y]; y++) {
			for (int z = 0; z < world->size[Z]; z++) {
				int cidx = world_lin(world->size, x, y, z);
				Chunk *chunk = &world->chunks[cidx];
				glm_ivec3_copy((ivec3){x, y, z}, chunk->cpos);
				chunk_init(chunk);
				chunk_bake(chunk);
			}
		}
	}
}

void world_free(World *world) {
	for (int i = 0; i < world->chunks_len; i++) {
		chunk_free(&world->chunks[i]);
	}
	nfree(world->chunks);
}