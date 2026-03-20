#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>	
#include <glm/glm.hpp>

#include "Render.h"

struct CameraInputContext
{
	Render* render = nullptr;
	bool rotating = false;
	double lastX = 0.0;
	double lastY = 0.0;
	float zoomStep = 0.2f;
	float rotateSensitivity = 0.2f;
};

static void onScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	(void)xoffset;
	auto* ctx = static_cast<CameraInputContext*>(glfwGetWindowUserPointer(window));
	if (ctx == nullptr || ctx->render == nullptr) {
		return;
	}

	Camera* camera = ctx->render->getCamera();
	if (camera == nullptr) {
		return;
	}

	camera->SetFocal(0.0f, 0.0f, 0.0f);
	if (yoffset > 0.0) {
		camera->Forward(static_cast<float>(yoffset) * ctx->zoomStep);
	}
	else if (yoffset < 0.0) {
		camera->Backward(static_cast<float>(-yoffset) * ctx->zoomStep);
	}
}

static void onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	(void)mods;
	auto* ctx = static_cast<CameraInputContext*>(glfwGetWindowUserPointer(window));
	if (ctx == nullptr) {
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			ctx->rotating = true;
			glfwGetCursorPos(window, &ctx->lastX, &ctx->lastY);
		}
		else if (action == GLFW_RELEASE) {
			ctx->rotating = false;
		}
	}
}

static void onCursorPos(GLFWwindow* window, double xpos, double ypos)
{
	auto* ctx = static_cast<CameraInputContext*>(glfwGetWindowUserPointer(window));
	if (ctx == nullptr || ctx->render == nullptr || !ctx->rotating) {
		return;
	}

	Camera* camera = ctx->render->getCamera();
	if (camera == nullptr) {
		return;
	}

	const float dx = static_cast<float>(xpos - ctx->lastX);
	const float dy = static_cast<float>(ypos - ctx->lastY);
	ctx->lastX = xpos;
	ctx->lastY = ypos;

	camera->SetFocal(0.0f, 0.0f, 0.0f);
	camera->Rotate3D(glm::vec2(dx, dy) * ctx->rotateSensitivity);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

	Render render("Vulkan Demo", window);
	render.getCamera()->SetFocal(0.0f, 0.0f, 0.0f);

	CameraInputContext cameraInput{};
	cameraInput.render = &render;
	glfwSetWindowUserPointer(window, &cameraInput);
	glfwSetScrollCallback(window, onScroll);
	glfwSetMouseButtonCallback(window, onMouseButton);
	glfwSetCursorPosCallback(window, onCursorPos);

	Actor actor;
	actor.setInputData(Loader::loadModel("../Dependence/resource/obj/bunny.obj"));

	render.addActor(&actor);

	while (!glfwWindowShouldClose(window)) {
		render.draw();
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}