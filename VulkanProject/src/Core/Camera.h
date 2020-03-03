#pragma once

#include "jaspch.h"

class Camera 
{
public:
	struct Plane
	{
		alignas(16) glm::vec3 normal;
		alignas(16) glm::vec3 point;
	};
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

	const std::vector<Plane>& getPlanes() const;

private:
	enum Side { NEAR_P = 0, FAR_P, LEFT_P, RIGHT_P, TOP_P, BOTTOM_P };

private:
	void updatePlanes();
	std::vector<Camera::Plane> planes;

	float speed, speedFactor;
	float fov;
	float nearPlane, farPlane;
	float yaw, pitch, roll;
	float aspect;
	float nearHeight, nearWidth, farHeight, farWidth;

	const glm::vec3 globalUp;

	glm::vec3 up;
	glm::vec3 forward;
	glm::vec3 right;

	glm::vec3 position;
	glm::vec3 target;
};