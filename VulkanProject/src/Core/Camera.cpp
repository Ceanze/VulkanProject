#include "jaspch.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/norm.hpp"
#include "Input.h"

#define FRUSTUM_SHRINK_FACTOR -5.f

Camera::Camera(float aspect, float fov, const glm::vec3& position, const glm::vec3& target, float speed, float hasteSpeed)
	: position(position), target(target), speed(speed), hasteSpeed(hasteSpeed), globalUp(0.f, 1.f, 0.f), aspect(aspect)
{
	this->forward = glm::normalize(this->target - this->position);
	this->up = this->globalUp;
	this->right = glm::cross(this->forward, this->up);

	this->fov = fov;
	this->nearPlane = 0.001f;
	this->farPlane = 2000.f;
	this->yaw = 270;
	this->pitch = 0;
	this->roll = 0;
	this->speedFactor = 1.f;

	float tang = tan(this->fov / 2);
	this->nearHeight = this->nearPlane * tang * 2.0f;
	this->nearWidth = this->nearHeight * this->aspect;
	this->farHeight = this->farPlane * tang * 2.0f;
	this->farWidth = this->farHeight * this->aspect;

	this->planes.resize(6);
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
		this->position += this->globalUp * this->speed * dt * this->speedFactor;
	if (input.isKeyDown(GLFW_KEY_LEFT_CONTROL))
		this->position -= this->globalUp * this->speed * dt * this->speedFactor;

	if (input.isKeyDown(GLFW_KEY_LEFT_SHIFT))
		this->speedFactor = hasteSpeed;
	else
		this->speedFactor = 1.f;

	glm::vec2 cursorDelta = input.getCursorDelta();
	cursorDelta *= 0.1f;

	if (glm::length2(cursorDelta) > glm::epsilon<float>())
	{
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
		this->right = glm::normalize(glm::cross(this->forward, this->globalUp));
		this->up = glm::cross(this->right, this->forward);

	}
	updatePlanes();
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

glm::vec3 Camera::getPosition() const
{
	return this->position;
}

const std::vector<Camera::Plane>& Camera::getPlanes() const
{
	return this->planes;
}

void Camera::updatePlanes()
{
	glm::vec3 point;
	glm::vec3 nearCenter = this->position + this->forward * this->nearPlane;
	glm::vec3 farCenter = this->position + this->forward * this->farPlane;

	this->planes[NEAR_P].normal = this->forward;
	this->planes[NEAR_P].point = nearCenter;

	this->planes[FAR_P].normal = -this->forward;
	this->planes[FAR_P].point = farCenter;

	point = nearCenter + this->up * (this->nearHeight / 2) - this->right * (this->nearWidth / 2);
	this->planes[LEFT_P].normal = glm::normalize(glm::cross(point - this->position, this->up));
	this->planes[LEFT_P].point = point + this->planes[LEFT_P].normal * FRUSTUM_SHRINK_FACTOR;

	this->planes[TOP_P].normal = glm::normalize(glm::cross(point - this->position, this->right));
	this->planes[TOP_P].point = point;

	point = nearCenter - this->up * (this->nearHeight / 2) + this->right * (this->nearWidth / 2);
	this->planes[RIGHT_P].normal = glm::normalize(glm::cross(point - this->position, -this->up));
	this->planes[RIGHT_P].point = point + this->planes[RIGHT_P].normal * FRUSTUM_SHRINK_FACTOR;

	this->planes[BOTTOM_P].normal = glm::normalize(glm::cross(point - this->position, -this->right));
	this->planes[BOTTOM_P].point = point;
}
