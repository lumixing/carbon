#version 330 core

// in vec3 oCol;
in vec2 oOff;
in float oMul;

uniform sampler2D u_texture;

out vec4 FragColor;

void main() {
	FragColor = texture(u_texture, oOff) * oMul;
}