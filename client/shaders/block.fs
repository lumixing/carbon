#version 330 core

// in vec3 oCol;
in vec2 oOff;

uniform sampler2D u_texture;

out vec4 FragColor;

void main() {
	FragColor = texture(u_texture, oOff);
}