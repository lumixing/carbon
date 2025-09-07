#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "chunk.h"
#include "../util.h"
#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include "camera.h"
#include "gl.h"
#include "world.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

void mouse_callback(GLFWwindow *window, double x, double y);

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

	int w, h, c;
	unsigned char *dd = stbi_load("client/assets/kms.png", &w, &h, &c, 4);
	assert(dd != NULL);
	GLFWimage icons[1] = {
		{32, 32, dd},
	};
	glfwSetWindowIcon(window, 1, icons);
	// stbi_image_free(data);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("could not init glad\n");
		return -1;
	}

	unsigned int program = create_program_from_shaders("client/shaders/block.vs", "client/shaders/block.fs");
	defer { glDeleteProgram(program); }

	unsigned int img_program = create_program_from_shaders("client/shaders/img.vs", "client/shaders/img.fs");
	defer { glDeleteProgram(img_program); }

	unsigned int vao;
	glGenVertexArrays(1, &vao);
	defer { glDeleteVertexArrays(1, &vao); }
	glBindVertexArray(vao);

	World world;
	glm_ivec3_copy((ivec3){4, 1, 4}, world.size);
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

	int u_texture = glGetUniformLocation(img_program, "texture");
	int u_texture2 = glGetUniformLocation(program, "texture");

	mat4 proj;
	glm_perspective(45, 800./600, 0.001, 1000, proj);

	glEnable(GL_DEPTH_TEST);

	char window_title[1024] = "hello :D";
	double prev_time = 0;

	unsigned int texture;
	glGenTextures(1, &texture);
	glActiveTexture(texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, chans;
	unsigned char *data = stbi_load("client/assets/kms.png", &width, &height, &chans, 4);
	assert(data != NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	// glGenerateMipmap(GL_TEXTURE_2D);

	unsigned int img_vbo;
	glGenBuffers(1, &img_vbo);
	defer { glDeleteBuffers(1, &img_vbo); }

	float img_vertices[] = {
		0./4/20, 0./3/20, 0, 0, // 0
		0./4/20, 1./3/20, 0, 1, // 1
		1./4/20, 1./3/20, 1, 1, // 2
		1./4/20, 1./3/20, 1, 1, // 2
		1./4/20, 0./3/20, 1, 0, // 3
		0./4/20, 0./3/20, 0, 0, // 0
	};

	glBindBuffer(GL_ARRAY_BUFFER, img_vbo);
	glBufferData(GL_ARRAY_BUFFER, 6*4*sizeof(float), img_vertices, GL_STATIC_DRAW);

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
			// printf("!!! pressed !!!\n");
			for (int i = 0; i < 10; i++) {
				vec3 pos;
				glm_vec3_scale(camera.front, i, pos);
				glm_vec3_add(pos, camera.position, pos);
				glm_vec3_floor(pos, pos);
				if (pos[X] < 0 || pos[Y] < 0 || pos[Z] < 0) continue;
				// printf("%d -> %f %f %f\n", i, pos[X], pos[Y], pos[Z]);
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
					// printf("hit block %d %d\n", ci, bi);
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
		glUniform1i(u_texture2, 0);

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

		glUseProgram(img_program);
		// glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(u_texture, 0);
		glBindBuffer(GL_ARRAY_BUFFER, img_vbo);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, false, 4*sizeof(float), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, 4*sizeof(float), (void *)(2*sizeof(float)));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
	}

	return 0;
}

void mouse_callback(GLFWwindow *window, double x, double y) {
	update_camera_rotation(window, x, y);
}
