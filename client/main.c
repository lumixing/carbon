#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "../defer.h"
#include "chunk.h"
#include "util.h"
#include "glad/include/glad/glad.h"
#include "glfw3.h"
#include "cglm/include/cglm/cglm.h"

#define DEBUG

#ifdef DEBUG

#include <windows.h>
#include <psapi.h>

#endif

float get_ram_usage_in_mb() {
#ifdef DEBUG
	PROCESS_MEMORY_COUNTERS pmc;
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		// printf("RAM: %.1f MB\n", pmc.WorkingSetSize / 1024.0 / 1024.0);
		return pmc.WorkingSetSize / 1024.0 / 1024.0;
	}
#endif
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

void update_camera_position(GLFWwindow *window) {
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
}

typedef struct {
	ivec3 size;
	int chunks_len;
	Chunk *chunks;
} World;

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
	// glfwSwapInterval(1);
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

	unsigned int vao;
	glGenVertexArrays(1, &vao);
	defer { glDeleteVertexArrays(1, &vao); }
	glBindVertexArray(vao);

	World world;
	glm_ivec3_copy((ivec3){8, 1, 8}, world.size);
	world_init(&world);
	defer { world_free(&world); }

	glm_vec3_copy((vec3){0, 0, 5}, camera.position);
	glm_vec3_copy((vec3){0, 0, -1}, camera.front);
	glm_vec3_copy((vec3){0, 1, 0}, camera.up);
	camera.yaw = 270;

	int u_model = glGetUniformLocation(program, "model");
	int u_view = glGetUniformLocation(program, "view");
	int u_proj = glGetUniformLocation(program, "proj");

	int u_cpos = glGetUniformLocation(program, "cpos");

	mat4 proj;
	glm_perspective(45, 800./600, 0.1, 1000, proj);

	glEnable(GL_DEPTH_TEST);

	char window_title[1024] = "hello :D";
	double prev_time = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		double time = glfwGetTime();
		float dt = time - prev_time;
		dt += 0; // dont bitch and moan
		prev_time = time;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		update_camera_position(window);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			printf("!!! pressed !!!\n");
			for (int i = 0; i < 10; i++) {
				vec3 pos;
				glm_vec3_scale(camera.front, i, pos);
				glm_vec3_add(pos, camera.position, pos);
				glm_vec3_floor(pos, pos);
				if (pos[X] < 0 || pos[Y] < 0 || pos[Z] < 0) continue;
				printf("%d -> %f %f %f\n", i, pos[X], pos[Y], pos[Z]);
				// todo: check world size
				int cx = pos[X] / CHUNK_SIZE;
				int cy = pos[Y] / CHUNK_SIZE;
				int cz = pos[Z] / CHUNK_SIZE;
				int ci = world_lin(world.size, cx, cy, cz);
				int x = (int)pos[X] % CHUNK_SIZE;
				int y = (int)pos[Y] % CHUNK_SIZE;
				int z = (int)pos[Z] % CHUNK_SIZE;
				int bi = chunk_lin(x, y, z);
				if (world.chunks[ci].blocks[bi] != 0) {
					world.chunks[ci].blocks[bi] = 0;
					printf("hit block %d %d\n", ci, bi);
					chunk_bake(&world.chunks[ci]);
					break;
				}
			}
		}

		// sprintf(window_title, "pos: %.2f %.2f %.2f (%.2f %.2f)\n", camera.position[0], camera.position[1], camera.position[2], camera.yaw, camera.pitch);
		sprintf(window_title, "%.0f fps / %.1f mb", 1./dt, get_ram_usage_in_mb());
		glfwSetWindowTitle(window, window_title);

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

		// chunk_render(&chunk, u_cpos);

		for (int i = 0; i < world.chunks_len; i++) {
			Chunk *chunk = &world.chunks[i];
			chunk_render(chunk, u_cpos);
		}

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