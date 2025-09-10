#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "chunk.h"

// stbiw's Y variable inteferes with util's Y macro (fuck you)
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "../include/stb_image_write.h"

#include "../util.h"
#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include "camera.h"
#include "gl.h"
#include "world.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../include/stb_truetype.h"

void mouse_callback(GLFWwindow *window, double x, double y);

#define FONT_SIZE 64

typedef struct {
	int width;
	int height;
	int channels;
	unsigned char *data;
} Image;

Image image_load(const char *path) {
	Image image;
	image.data = stbi_load(path, &image.width, &image.height, &image.channels, 4);
	assert(image.data);

	return image;
}

void image_free(const Image *image) {
	stbi_image_free(image->data);
}

void draw_text(int bw, int bh, stbtt_packedchar chardata[], unsigned int program, unsigned int font_texture, int u_texture, const char *text, float x, float y) {
	unsigned int vbo, ebo;

	glGenBuffers(1, &vbo);
	defer { glDeleteBuffers(1, &vbo); }

	glGenBuffers(1, &ebo);
	defer { glDeleteBuffers(1, &ebo); }

	int text_len = strlen(text);

	float *vertices = malloc(text_len * 4 * 4 * sizeof(float));
	defer { nfree(vertices); }

	float advance = 0;

	for (int i = 0; i < text_len; i++) {
		stbtt_packedchar ch = chardata[text[i] - 32];

		float width = ch.x1 - ch.x0;
		float height = ch.y1 - ch.y0;

		float x0n = (float)ch.x0 / bw;
		float y0n = (float)ch.y0 / bh;
		float x1n = (float)ch.x1 / bw;
		float y1n = (float)ch.y1 / bh;

		float char_vertices[4 * 4] = {
			0./800*width-1+advance/800+x/800, 0./600*height+1-(float)FONT_SIZE/600-height/600-ch.yoff/600-y/600, x0n, y1n, // 0
			0./800*width-1+advance/800+x/800, 1./600*height+1-(float)FONT_SIZE/600-height/600-ch.yoff/600-y/600, x0n, y0n, // 1
			1./800*width-1+advance/800+x/800, 1./600*height+1-(float)FONT_SIZE/600-height/600-ch.yoff/600-y/600, x1n, y0n, // 2
			1./800*width-1+advance/800+x/800, 0./600*height+1-(float)FONT_SIZE/600-height/600-ch.yoff/600-y/600, x1n, y1n, // 3
		};

		memcpy(vertices + (i * 4 * 4), char_vertices, 4 * 4 * sizeof(float));

		advance += ch.xadvance;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, text_len*4*4*sizeof(float), vertices, GL_STATIC_DRAW);

	unsigned int *indices = malloc(text_len * 6 * sizeof(unsigned int));
	defer { nfree(indices); }

	for (int i = 0; i < text_len; i++) {
		unsigned int char_indices[6] = {
			0 + i * 4, 1 + i * 4, 2 + i * 4,
			2 + i * 4, 3 + i * 4, 0 + i * 4,
		};

		memcpy(indices + i * 6, char_indices, 6 * sizeof(unsigned int));
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, text_len*6*sizeof(unsigned int), indices, GL_STATIC_DRAW);

	// draw
	glUseProgram(program);

	glBindTexture(GL_TEXTURE_2D, font_texture);
	glUniform1i(u_texture, 0);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 4*sizeof(float), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 4*sizeof(float), (void *)(2*sizeof(float)));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDrawElements(GL_TRIANGLES, text_len * 6, GL_UNSIGNED_INT, 0);

	glDisable(GL_BLEND);

}

int main() {
	unsigned char *font_buffer;
	read_entire_file((char **)&font_buffer, "client/assets/hack.ttf");
	defer { free(font_buffer); }

	stbtt_fontinfo font;
	stbtt_InitFont(&font, font_buffer, 0);

	int bw = 1024, bh = 1024;
	unsigned char *bitmap = malloc(bw * bh);
	defer { free(bitmap); }

	stbtt_pack_context pc;
	stbtt_PackBegin(&pc, bitmap, bw, bh, 0, 1, NULL);
	stbtt_packedchar chardata[96];
	stbtt_PackFontRange(&pc, font_buffer, 0, FONT_SIZE, 32, 96, chardata);
	stbtt_PackEnd(&pc);

	// for (int i = 0; i < 96; i++) {
	// 	stbtt_packedchar c = chardata[i];
	// 	printf("%u;%u;%u;%u\n", c.x0, c.y0, c.x1, c.y1);
	// }

	// stbi_write_png("font2.png", bw, bh, 1, bitmap, bw);

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

	Image kms_img = image_load("client/assets/kms.png");
	defer { image_free(&kms_img); }

	// Image font_img = image_load("client/assets/hack.png");
	// defer { image_free(&font_img); }

	GLFWimage icons[1] = {{kms_img.width, kms_img.height, kms_img.data}};
	glfwSetWindowIcon(window, 1, icons);

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
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kms_img.width, kms_img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, kms_img.data);

	unsigned int font_texture;
	glGenTextures(1, &font_texture);
	glBindTexture(GL_TEXTURE_2D, font_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bw, bh, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

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

		sprintf(window_title, "pos: %.2f %.2f %.2f (%.2f %.2f)\n", camera.position[0], camera.position[1], camera.position[2], camera.yaw, camera.pitch);
		// sprintf(window_title, "%.0f fps / %.1f mb", 1./dt, get_ram_usage_in_mb());
		glfwSetWindowTitle(window, window_title);

		glClearColor(135./255, 206./255, 235./255, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program);

		// glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
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

		for (int i = 0; i < world.chunks_len; i++) {
			Chunk *chunk = &world.chunks[i];
			chunk_render(chunk, u_cpos);
		}

		draw_text(bw, bh, chardata, img_program, font_texture, u_texture, "hello world B)", 0, 0);
		draw_text(bw, bh, chardata, img_program, font_texture, u_texture, window_title, 64, 64);

		glfwSwapBuffers(window);
	}

	return 0;
}

void mouse_callback(GLFWwindow *window, double x, double y) {
	update_camera_rotation(window, x, y);
}
