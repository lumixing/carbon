#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../defer.h"
#include "glad/include/glad/glad.h"
#include "glfw3.h"
#include "cglm/include/cglm/cglm.h"

#include <windows.h>
#include <psapi.h>

float get_ram_usage_in_mb() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        // printf("RAM: %.1f MB\n", pmc.WorkingSetSize / 1024.0 / 1024.0);
		return pmc.WorkingSetSize / 1024.0 / 1024.0;
    }
	return -1;
}

void print_ram(char *text) {
	printf("RAM USAGE (%s): %.2f MB\n", text, get_ram_usage_in_mb());
}

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

typedef struct {
	vec3 position;
	vec3 front;
	vec3 up;
	float yaw;
	float pitch;
} Camera;

Camera camera = {0};

#define X 0
#define Y 1
#define Z 2

#define CAM_SPEED 0.2

void mouse_callback(GLFWwindow *window, double x, double y);

typedef enum {
	BLOCK_AIR,
	BLOCK_GRASS,
	BLOCK_DIRT,
	BLOCK_STONE,
} Block;

#define CHUNK_SIZE 32

int chunk_lin(int x, int y, int z) {
	return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
}

typedef struct {
	char *blocks;
	int vertices_len;
	float *vertices;
	int indices_len;
	unsigned int *indices;
} Chunk;

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

void chunk_bake_block(Chunk *chunk, int x, int y, int z, int block_count) {
	float vertices[] = {
		0+x, 1+y, 0+z, 0, 1, 0, // front
		1+x, 1+y, 0+z, 0, 1, 0,
		0+x, 0+y, 0+z, 0, 1, 0,
		1+x, 0+y, 0+z, 0, 1, 0,
		0+x, 1+y, 1+z, 0, 1, 0, // back
		1+x, 1+y, 1+z, 0, 1, 0,
		0+x, 0+y, 1+z, 0, 1, 0,
		1+x, 0+y, 1+z, 0, 1, 0,
		0+x, 1+y, 0+z, 0, 0, 1, // left
		0+x, 0+y, 0+z, 0, 0, 1,
		0+x, 1+y, 1+z, 0, 0, 1,
		0+x, 0+y, 1+z, 0, 0, 1,
		1+x, 1+y, 0+z, 0, 0, 1, // right
		1+x, 0+y, 0+z, 0, 0, 1,
		1+x, 1+y, 1+z, 0, 0, 1,
		1+x, 0+y, 1+z, 0, 0, 1,
		0+x, 0+y, 0+z, 1, 0, 0, // bottom
		1+x, 0+y, 0+z, 1, 0, 0,
		0+x, 0+y, 1+z, 1, 0, 0,
		1+x, 0+y, 1+z, 1, 0, 0,
		0+x, 1+y, 0+z, 1, 0, 0, // top
		1+x, 1+y, 0+z, 1, 0, 0,
		0+x, 1+y, 1+z, 1, 0, 0,
		1+x, 1+y, 1+z, 1, 0, 0,
	};

	// printf("%d %u %u\n", block_count, chunk->vertices, chunk->vertices+(block_count * 6 * 4 * 6 * sizeof(float)));
	memcpy(chunk->vertices + (block_count * 6 * 4 * 6), vertices, 6 * 4 * 6 * sizeof(float));
}

// make sure to free vertices and indices if needed before (or after) baking
void chunk_bake(Chunk *chunk) {
	int block_count = 0;

	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++) {
		if (chunk->blocks[i] != 0) {
			block_count += 1;
		}
	}

	printf("allocating %lld kb for vertices\n", block_count * 6 * 4 * 6 * sizeof(float)/1024);
	chunk->vertices = malloc(block_count * 6 * 4 * 6 * sizeof(float));
	chunk->vertices_len = block_count * 6 * 4 * 6;
	printf("allocating %lld kb for indices\n", block_count * 6 * 6 * sizeof(unsigned int)/1024);
	chunk->indices = malloc(block_count * 6 * 6 * sizeof(unsigned int));
	chunk->indices_len = block_count * 6 * 6;

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

void chunk_free(Chunk *chunk) {
	free(chunk->blocks);
	free(chunk->vertices);
	free(chunk->indices);
}

int main() {
	if (!glfwInit()) {
		printf("could not init glfw\n");
		return -1;
	}
	defer { glfwTerminate(); }

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(800, 600, "carbon client", NULL, NULL);
	if (!window) {
		printf("could not init window\n");
		return -1;
	}
	defer { glfwDestroyWindow(window); }

	glfwSetWindowPos(window, (1920-800)/2, (1080-600)/2);
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("could not init glad\n");
		return -1;
	}

	char *vertex_src;
	if (!read_entire_file(&vertex_src, "client/shaders/triangle.vs")) {
		printf("could not read .vs\n");
		return -1;
	}

	char *fragment_src;
	if (!read_entire_file(&fragment_src, "client/shaders/triangle.fs")) {
		printf("could not read .fs\n");
		return -1;
	}
	
	int success;
	char info_log[1024];

	unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, (const char **)&vertex_src, NULL);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex_shader, 1024, NULL, info_log);
		printf("could not compile .vs: %s\n", info_log);
		return -1;
	}

	unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, (const char **)&fragment_src, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, 1024, NULL, info_log);
		printf("could not compile .fs: %s\n", info_log);
		return -1;
	}

	unsigned int program = glCreateProgram();
	defer { glDeleteProgram(program); }

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 1024, NULL, info_log);
		printf("could not link program: %s\n", info_log);
		return -1;
	}

	free(vertex_src);
	glDeleteShader(vertex_shader);
	free(fragment_src);
	glDeleteShader(fragment_shader);

	Chunk chunk;
	chunk_init(&chunk);
	print_ram("before baking");
	chunk_bake(&chunk);
	print_ram("after baking");
	free(chunk.blocks);
	print_ram("after freeing blocks");

	unsigned int vao, vbo, ebo;

	glGenVertexArrays(1, &vao);
	defer { glDeleteVertexArrays(1, &vao); }

	glGenBuffers(1, &vbo);
	defer { glDeleteBuffers(1, &vbo); }

	glGenBuffers(1, &ebo);
	defer { glDeleteBuffers(1, &ebo); }

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, chunk.vertices_len * sizeof(float), chunk.vertices, GL_STATIC_DRAW);
	free(chunk.vertices);
	print_ram("after freeing vertices");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunk.indices_len * sizeof(unsigned int), chunk.indices, GL_STATIC_DRAW);
	free(chunk.indices);
	print_ram("after freeing indices");

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Camera camera = {0};
	glm_vec3_copy((vec3){0, 0, 5}, camera.position);
	glm_vec3_copy((vec3){0, 0, -1}, camera.front);
	glm_vec3_copy((vec3){0, 1, 0}, camera.up);
	camera.yaw = 270;

	int u_model = glGetUniformLocation(program, "model");
	int u_view = glGetUniformLocation(program, "view");
	int u_proj = glGetUniformLocation(program, "proj");

	mat4 proj;
	glm_perspective(45, 800./600, 0.1, 1000, proj);
	// glUniformMatrix4fv(u_proj, 1, GL_FALSE, proj[0]);

	glEnable(GL_DEPTH_TEST);

	char window_title[1024] = "hello :D";
	double prev_time = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		double time = glfwGetTime();
		float dt = time - prev_time;
		prev_time = time;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			vec2 front;
			glm_vec2_copy((vec2){camera.front[X], camera.front[Z]}, front);
			glm_vec2_normalize(front);
			camera.position[X] += front[X] * CAM_SPEED;
			camera.position[Z] += front[Y] * CAM_SPEED;
		}

		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			vec2 front;
			glm_vec2_copy((vec2){camera.front[X], camera.front[Z]}, front);
			glm_vec2_normalize(front);
			camera.position[X] -= front[X] * CAM_SPEED;
			camera.position[Z] -= front[Y] * CAM_SPEED;
		}

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			vec3 cross;
			glm_vec3_cross(camera.front, camera.up, cross);
			glm_vec3_normalize(cross);
			glm_vec3_scale(cross, CAM_SPEED, cross);
			glm_vec3_sub(camera.position, cross, camera.position);
		}

		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			vec3 cross;
			glm_vec3_cross(camera.front, camera.up, cross);
			glm_vec3_normalize(cross);
			glm_vec3_scale(cross, CAM_SPEED, cross);
			glm_vec3_add(camera.position, cross, camera.position);
		}

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			vec3 up;
			glm_vec3_scale(camera.up, CAM_SPEED, up);
			glm_vec3_add(camera.position, up, camera.position);
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			vec3 up;
			glm_vec3_scale(camera.up, CAM_SPEED, up);
			glm_vec3_sub(camera.position, up, camera.position);
		}

		// sprintf(window_title, "pos: %.2f %.2f %.2f (%.2f %.2f)\n", camera.position[0], camera.position[1], camera.position[2], camera.yaw, camera.pitch);
		sprintf(window_title, "%.0f fps / %.1f mb", 1./dt, get_ram_usage_in_mb());
		glfwSetWindowTitle(window, window_title);

		// printf("pos: %.2f %.2f %.2f (%.2f %.2f)\n", camera.position[0], camera.position[1], camera.position[2], camera.yaw, camera.pitch);
		// print_ram_usage();

		glClearColor(135./255, 206./255, 235./255, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program);
		glBindVertexArray(vao);

		glUniformMatrix4fv(u_proj, 1, GL_FALSE, proj[0]);

		vec3 center;
		glm_vec3_add(camera.position, camera.front, center);
		mat4 view;
		glm_lookat(camera.position, center, camera.up, view);
		glUniformMatrix4fv(u_view, 1, GL_FALSE, view[0]);

		mat4 model;
		glm_mat4_identity(model);
		glUniformMatrix4fv(u_model, 1, GL_FALSE, model[0]);

		glDrawElements(GL_TRIANGLES, chunk.indices_len, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
	}

	return 0;
}

#define SENS 0.1

void mouse_callback(GLFWwindow *window, double x, double y) {
	// not using cglm for ts /shrug
	static double last_x = 0;
	static double last_y = 0;
	static bool first = true;

	if (first) {
		first = false;
		last_x = x;
		last_y = y;
	}

	double offset_x = x - last_x;
	double offset_y = -(y - last_y);

	last_x = x;
	last_y = y;

	offset_x *= SENS;
	offset_y *= SENS;

	camera.yaw += offset_x;
	camera.pitch += offset_y;
	if (camera.pitch < -89) camera.pitch = -89;
	else if (camera.pitch > 89) camera.pitch = 89;

	camera.front[X] = cosf(glm_rad(camera.yaw)) * cosf(glm_rad(camera.pitch));
	camera.front[Y] = sinf(glm_rad(camera.pitch));
	camera.front[Z] = sinf(glm_rad(camera.yaw)) * cosf(glm_rad(camera.pitch));

	glm_vec3_normalize(camera.front);
}