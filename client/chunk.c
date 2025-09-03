#include "chunk.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int chunk_lin(int x, int y, int z) {
	return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
}

void chunk_init(Chunk *chunk) {
	chunk->blocks = malloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

	if (chunk->blocks == NULL) {
		printf("could not init chunk, buy more ram.\n");
		return;
	}

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				int i = chunk_lin(x, y, z);
				chunk->blocks[i] = rand() % 2;
			}
		}
	}
}

void chunk_free(Chunk *chunk) {
	free(chunk->blocks);
	free(chunk->vertices);
	free(chunk->indices);
}

void chunk_bake_block(Chunk *chunk, int x, int y, int z, int block_count) {
	BlockVertex vertices[] = {
		{0+x, 1+y, 0+z, 0}, // front
		{1+x, 1+y, 0+z, 0},
		{0+x, 0+y, 0+z, 0},
		{1+x, 0+y, 0+z, 0},
		{0+x, 1+y, 1+z, 0}, // back
		{1+x, 1+y, 1+z, 0},
		{0+x, 0+y, 1+z, 0},
		{1+x, 0+y, 1+z, 0},
		{0+x, 1+y, 0+z, 1}, // left
		{0+x, 0+y, 0+z, 1},
		{0+x, 1+y, 1+z, 1},
		{0+x, 0+y, 1+z, 1},
		{1+x, 1+y, 0+z, 1}, // right
		{1+x, 0+y, 0+z, 1},
		{1+x, 1+y, 1+z, 1},
		{1+x, 0+y, 1+z, 1},
		{0+x, 0+y, 0+z, 2}, // bottom
		{1+x, 0+y, 0+z, 2},
		{0+x, 0+y, 1+z, 2},
		{1+x, 0+y, 1+z, 2},
		{0+x, 1+y, 0+z, 2}, // top
		{1+x, 1+y, 0+z, 2},
		{0+x, 1+y, 1+z, 2},
		{1+x, 1+y, 1+z, 2},
	};

	memcpy(chunk->vertices + (block_count * 4 * 6), vertices, 4 * 6 * sizeof(BlockVertex));
}

// make sure to free vertices and indices if needed before (or after) baking
void chunk_bake(Chunk *chunk) {
	int block_count = 0;

	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++) {
		if (chunk->blocks[i] != 0) {
			block_count += 1;
		}
	}

	chunk->vertices = malloc(block_count * 6 * 4 * sizeof(BlockVertex));
	chunk->vertices_len = block_count * 6 * 4;
	// printf("allocating %lld b for %d vertices\n", block_count * 6 * 4 * sizeof(BlockVertex), block_count * 6 * 4);
	chunk->indices = malloc(block_count * 6 * 6 * sizeof(unsigned int));
	chunk->indices_len = block_count * 6 * 6;
	// printf("allocating %lld b for %d indices\n", block_count * 6 * 6 * sizeof(unsigned int), block_count * 6 * 6);

	for (int i = 0; i < block_count * 6; i++) {
		chunk->indices[0+6*i] = 0+4*i;
		chunk->indices[1+6*i] = 1+4*i;
		chunk->indices[2+6*i] = 3+4*i;
		chunk->indices[3+6*i] = 0+4*i;
		chunk->indices[4+6*i] = 3+4*i;
		chunk->indices[5+6*i] = 2+4*i;
	}

	block_count = 0;
	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				int i = chunk_lin(x, y, z);
				if (chunk->blocks[i] == 0) {
					continue;
				}
				chunk_bake_block(chunk, x, y, z, block_count++);
			}
		}
	}
}