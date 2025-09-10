#version 330 core

in vec2 o_off;

uniform sampler2D u_texture;

out vec4 o_color;

void main() {
	float alpha = texture(u_texture, o_off).r;
	o_color = vec4(1.0, 1.0, 1.0, alpha);
}