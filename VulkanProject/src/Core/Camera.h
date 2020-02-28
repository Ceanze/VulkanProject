#pragma once

#include "jaspch.h"

class Camera 
{
public:
	Camera(float aspect, float fov, const glm::vec3& position, const glm::vec3& target, float speed);
	~Camera();
	
	void update(float dt);

	void setPosition(const glm::vec3& position);
	void setSpeed(float speed);

	glm::mat4 getMatrix() const;
	glm::mat4 getProjection() const;
	glm::mat4 getView() const;
	glm::vec3 getPosition() const;

private:
	float speed, speedFactor;
	float fov;
	float nearPlane, farPlane;
	float yaw, pitch, roll;
	float aspect;

	const glm::vec3 globalUp;

	glm::vec3 up;
	glm::vec3 forward;
	glm::vec3 right;

	glm::vec3 position;
	glm::vec3 target;
};