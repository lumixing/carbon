#include "../util.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define CAM_SPEED 0.2
#define SENS 0.1

typedef struct {
	vec3 position;
	vec3 front;
	vec3 up;
	float yaw;
	float pitch;
} Camera;

extern Camera camera;

void update_camera_rotation(GLFWwindow *window, double x, double y);
void update_camera_position(GLFWwindow *window);
