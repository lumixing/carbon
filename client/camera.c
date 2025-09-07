#include "camera.h"

Camera camera = {0};

void update_camera_rotation(GLFWwindow *window, double x, double y) {
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