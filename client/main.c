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

int main() {
	char *font_buffer;
	read_entire_file(&font_buffer, "client/assets/hack.ttf");
	defer { free(font_buffer); }

	stbtt_fontinfo font;
	stbtt_InitFont(&font, font_buffer, 0);

	int bw = 256, bh = 256;
	char *bitmap = malloc(bw * bh);
	defer { free(bitmap); }

	stbtt_pack_context pc;
	stbtt_PackBegin(&pc, bitmap, bw, bh, 0, 1, NULL);
	stbtt_packedchar chardata[96];
	stbtt_PackFontRange(&pc, font_buffer, 0, 32, 32, 96, chardata);
	stbtt_PackEnd(&pc);

	// for (int i = 0; i < 96; i++) {
	// 	stbtt_packedchar c = chardata[i];
	// 	printf("%u;%u;%u;%u\n", c.x0, c.y0, c.x1, c.y1);
	// }

	// stbi_write_png("font.png", bw, bh, 1, bitmap, bw);

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

	Image font_img = image_load("client/assets/hack.png");
	defer { image_free(&font_img); }

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font_img.width, font_img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, font_img.data);

	unsigned int img_vbo, img_ebo;

	glGenBuffers(1, &img_vbo);
	defer { glDeleteBuffers(1, &img_vbo); }

	glGenBuffers(1, &img_ebo);
	defer { glDeleteBuffers(1, &img_ebo); }

	//1;107;14;128
	// int x0 = 1;
	// int y0 = 107;
	// int x1 = 14;
	// int y1 = 128;

	// float x0n = 1./256;
	// float y0n = 107./256;
	// float x1n = 14./256;
	// float y1n = 128./256;

	// int w0 = x1 - x0;
	// int h0 = y1 - y0;

	stbtt_packedchar hc = chardata['h'-32];
	int hw0 = hc.x1 - hc.x0;
	int hh0 = hc.y1 - hc.y0;
	float hx0n = (float)hc.x0/256;
	float hy0n = (float)hc.y0/256;
	float hx1n = (float)hc.x1/256;
	float hy1n = (float)hc.y1/256;

	stbtt_packedchar ec = chardata['e'-32];
	int ew0 = ec.x1 - ec.x0;
	int eh0 = ec.y1 - ec.y0;
	float ex0n = (float)ec.x0/256;
	float ey0n = (float)ec.y0/256;
	float ex1n = (float)ec.x1/256;
	float ey1n = (float)ec.y1/256;

	stbtt_packedchar yc = chardata['y'-32];
	int yw0 = yc.x1 - yc.x0;
	int yh0 = yc.y1 - yc.y0;
	float yx0n = (float)yc.x0/256;
	float yy0n = (float)yc.y0/256;
	float yx1n = (float)yc.x1/256;
	float yy1n = (float)yc.y1/256;

	printf("%f %f %f\n", hc.yoff, ec.yoff, yc.yoff);

	float img_vertices[] = {
		0./800*hw0*10-1, 0.5+0./600*hh0*10-1-(float)hh0/600*10-hc.yoff/600*10, hx0n, hy1n, // 0
		0./800*hw0*10-1, 0.5+1./600*hh0*10-1-(float)hh0/600*10-hc.yoff/600*10, hx0n, hy0n, // 1
		1./800*hw0*10-1, 0.5+1./600*hh0*10-1-(float)hh0/600*10-hc.yoff/600*10, hx1n, hy0n, // 2
		1./800*hw0*10-1, 0.5+0./600*hh0*10-1-(float)hh0/600*10-hc.yoff/600*10, hx1n, hy1n, // 3

		0./800*ew0*10-1+(float)(hw0)/800*10, 0.5+0./600*eh0*10-1-(float)eh0/600*10-ec.yoff/600*10, ex0n, ey1n, // 0
		0./800*ew0*10-1+(float)(hw0)/800*10, 0.5+1./600*eh0*10-1-(float)eh0/600*10-ec.yoff/600*10, ex0n, ey0n, // 1
		1./800*ew0*10-1+(float)(hw0)/800*10, 0.5+1./600*eh0*10-1-(float)eh0/600*10-ec.yoff/600*10, ex1n, ey0n, // 2
		1./800*ew0*10-1+(float)(hw0)/800*10, 0.5+0./600*eh0*10-1-(float)eh0/600*10-ec.yoff/600*10, ex1n, ey1n, // 3

		0./800*yw0*10-1+(float)(hw0+ew0)/800*10, 0.5+0./600*yh0*10-1-(float)yh0/600*10-yc.yoff/600*10, yx0n, yy1n, // 0
		0./800*yw0*10-1+(float)(hw0+ew0)/800*10, 0.5+1./600*yh0*10-1-(float)yh0/600*10-yc.yoff/600*10, yx0n, yy0n, // 1
		1./800*yw0*10-1+(float)(hw0+ew0)/800*10, 0.5+1./600*yh0*10-1-(float)yh0/600*10-yc.yoff/600*10, yx1n, yy0n, // 2
		1./800*yw0*10-1+(float)(hw0+ew0)/800*10, 0.5+0./600*yh0*10-1-(float)yh0/600*10-yc.yoff/600*10, yx1n, yy1n, // 3
	};

	glBindBuffer(GL_ARRAY_BUFFER, img_vbo);
	glBufferData(GL_ARRAY_BUFFER, 12*4*sizeof(float), img_vertices, GL_STATIC_DRAW);

	unsigned int img_indices[] = {
		0, 1, 2,
		2, 3, 0,

		0+4, 1+4, 2+4,
		2+4, 3+4, 0+4,

		0+8, 1+8, 2+8,
		2+8, 3+8, 0+8,
	};

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, img_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 18*sizeof(unsigned int), img_indices, GL_STATIC_DRAW);

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

		// chunk_render(&chunk, u_cpos);

		for (int i = 0; i < world.chunks_len; i++) {
			Chunk *chunk = &world.chunks[i];
			chunk_render(chunk, u_cpos);
		}

		glUseProgram(img_program);

		// glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, font_texture);
		glUniform1i(u_texture, 0);
		
		glBindBuffer(GL_ARRAY_BUFFER, img_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, img_ebo);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, false, 4*sizeof(float), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, 4*sizeof(float), (void *)(2*sizeof(float)));

		// glDrawArrays(GL_TRIANGLES, 0, 6);
		glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
	}

	return 0;
}

void mouse_callback(GLFWwindow *window, double x, double y) {
	update_camera_rotation(window, x, y);
}
