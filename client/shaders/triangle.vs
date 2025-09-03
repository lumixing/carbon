#version 400 core

layout(location = 0) in uint data;

//layout(location = 0) in vec3 aPos;
//layout(location = 1) in vec3 aCol;

out vec3 oCol;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
	vec3 aPos = vec3(
		bitfieldExtract(data, 0, 5),
		bitfieldExtract(data, 5, 5),
		bitfieldExtract(data, 10, 5)
	);

	gl_Position = proj * view * model * vec4(aPos, 1);
	//oCol = aCol;

	uint col = bitfieldExtract(data, 15, 2);
	if (col == 0) oCol = vec3(1, 0, 0);
	else if (col == 1) oCol = vec3(0, 1, 0);
	else if (col == 2) oCol = vec3(0, 0, 1);
	else if (col == 3) oCol = vec3(1, 1, 0);
	else if (col < 0) oCol = vec3(0, 1, 1);
	else oCol = vec3(1, 0, 1);
}