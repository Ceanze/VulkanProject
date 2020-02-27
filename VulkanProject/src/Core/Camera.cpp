#include "jaspch.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Input.h"

Camera::Camera(float aspect, float fov, const glm::vec3& position, const glm::vec3& target, float speed)
	: position(position), target(target), speed(speed), globalUp(0.f, 1.f, 0.f), aspect(aspect)
{
	this->forward = this->target - this->position;
	this->up = this->globalUp;
	this->right = glm::cross(this->forward, this->up);
	this->fov = fov;
	this->nearPlane = 0.001f;
	this->farPlane = 100.f;
	this->yaw = 270;
	this->pitch = 0;
	this->roll = 0;
	this->speedFactor = 1.f;
}

Camera::~Camera()
{
}

void Camera::update(float dt)
{
	static Input& input = Input::get();

	if (input.isKeyDown(GLFW_KEY_A))
		this->position -= this->right * this->speed * dt * this->speedFactor;
	if(input.isKeyDown(GLFW_KEY_D))
		this->position += this->right * this->speed * dt * this->speedFactor;

	if (input.isKeyDown(GLFW_KEY_W))
		this->position += this->forward * this->speed * dt * this->speedFactor;
	if (input.isKeyDown(GLFW_KEY_S))
		this->position -= this->forward * this->speed * dt* this->speedFactor;

	if (input.isKeyDown(GLFW_KEY_SPACE))
		this->position += this->up * this->speed * dt * this->speedFactor;
	if (input.isKeyDown(GLFW_KEY_LEFT_CONTROL))
		this->position -= this->up * this->speed * dt * this->speedFactor;

	if (input.isKeyDown(GLFW_KEY_LEFT_SHIFT))
		this->speedFactor = 5.f;
	else
		this->speedFactor = 1.f;

	glm::vec2 cursorDelta = input.getCursorDelta();
	cursorDelta *= 0.1f;

	this->yaw += cursorDelta.x;
	this->pitch -= cursorDelta.y;

	if (this->pitch > 89.0f)
		this->pitch = 89.0f;
	if (this->pitch < -89.0f)
		this->pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
	direction.y = sin(glm::radians(this->pitch));
	direction.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));

	this->forward = glm::normalize(direction);
	this->right = glm::cross(this->forward, this->globalUp);
	this->up = glm::cross(this->right, this->forward);
	this->target = this->position + this->forward;
}

void Camera::setPosition(const glm::vec3& position)
{
	this->position = position;
}

void Camera::setSpeed(float speed)
{
	this->speed = speed;
}

glm::mat4 Camera::getMatrix() const
{
	return getProjection()* getView();
}

glm::mat4 Camera::getProjection() const
{
	glm::mat4 proj = glm::perspective(this->fov, this->aspect, this->nearPlane, this->farPlane);
	proj[1][1] *= -1;
	return proj;
}

glm::mat4 Camera::getView() const
{
	return glm::lookAt(this->position, this->target, this->up);
}
