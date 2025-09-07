#include "gl.h"

#include "../util.h"
#include <stdlib.h>
#include <stdio.h>
#include "../include/glad/glad.h"

unsigned int create_program_from_shaders(char *vs_path, char *fs_path) {
	char *vertex_src;
	if (!read_entire_file(&vertex_src, vs_path)) {
		printf("could not read %s\n", vs_path);
		return -1;
	}

	char *fragment_src;
	if (!read_entire_file(&fragment_src, fs_path)) {
		printf("could not read %s\n", fs_path);
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
		printf("could not compile %s: %s\n", vs_path, info_log);
		return -1;
	}

	unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, (const char **)&fragment_src, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, 1024, NULL, info_log);
		printf("could not compile %s: %s\n", fs_path, info_log);
		return -1;
	}

	unsigned int program = glCreateProgram();
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

	return program;
}