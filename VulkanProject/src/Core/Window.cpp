#include "jaspch.h"
#include "Window.h"
#include "Input.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

bool Window::open = false;

Window::Window()
{
}

Window::~Window()
{
}

void Window::init(unsigned width, unsigned height, const std::string& title, bool fullscreen)
{
	// Set values
	this->width = width;
	this->height = height;
	this->title = title;

	// Create window and init GLFW
	if (!glfwInit())
		JAS_FATAL("GLFW could not be initalized");

	// Create a window without OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
	// Disable window resize until vulkan renderer can handle it
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWmonitor* monitor = nullptr;
	if (fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();

		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		this->width = mode->width;
		this->height = mode->height;
	}

	this->window = glfwCreateWindow((int)this->width, (int)this->height, this->title.c_str(), monitor, nullptr);
	if (!this->window) {
		glfwTerminate();
		JAS_FATAL("Could not create a GLFW window!");
	}

	glfwMakeContextCurrent(this->window);

	this->open = true;

	// Callbacks
	glfwSetWindowCloseCallback(this->window, closeWindowCallback);
	glfwSetKeyCallback(this->window, keyCallback);
	glfwSetCursorPosCallback(this->window, mouseCallback);
}

void Window::cleanup()
{
	glfwDestroyWindow(this->window);
	glfwTerminate();
}

void Window::setTitle(const std::string& title)
{
	this->title = title;
	glfwSetWindowTitle(this->window, this->title.c_str());
}

void Window::closeWindowCallback(GLFWwindow* w)
{
	// TODO: Handle closing of window in renderer or similar
	Window::open = false;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE)
		Window::open = false;

	if (action == GLFW_PRESS)
		Input::get().setKeyPressed(key);
	else if (action == GLFW_RELEASE)
		Input::get().setKeyReleased(key);
}

void Window::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	Input &input = Input::get();
	if (input.isKeyToggled(GLFW_KEY_C)) {
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glfwSetCursorPos(window, (double)width * 0.5, (double)height * 0.5);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		// Send cursor offset from centre of window
		input.updateCursor(xpos - (double)width * 0.5, ypos - (double)height * 0.5);

	}
	else
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
