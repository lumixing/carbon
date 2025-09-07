#pragma once // todo: do pragma for all headers im tired by bye
#include <cglm/cglm.h>

#define CHUNK_SIZE 16

typedef enum {
	BLOCK_AIR,
	BLOCK_GRASS,
	BLOCK_DIRT,
	BLOCK_STONE,
} Block;

typedef struct {
	unsigned int x : 5;
	unsigned int y : 5;
	unsigned int z : 5;
	unsigned int b : 2;
	unsigned int t : 2;
} BlockVertex;

typedef struct {
	char *blocks;
	unsigned int vbo, ebo;
	int vertices_len;
	BlockVertex *vertices;
	int indices_len;
	unsigned int *indices;
	ivec3 cpos;
} Chunk;

int chunk_lin(int x, int y, int z);
void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);
void chunk_bake_block(Chunk *chunk, int x, int y, int z, int block_count);
void chunk_bake(Chunk *chunk);
void chunk_render(Chunk *chunk, int u_cpos);
