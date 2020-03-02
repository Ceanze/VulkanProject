#pragma once

#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Texture.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Vertex
{
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 nor;
	alignas(16) glm::vec2 uv0;

	static VkVertexInputBindingDescription getBindingDescriptions() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDescription.stride = sizeof(Vertex);
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
		// Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		// Normal
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, nor);
		// Uv0
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, uv0);
		return attributeDescriptions;
	}
};

struct Primitive
{
	uint32_t firstIndex{0};
	uint32_t indexCount{0};
	uint32_t vertexCount{0};
	bool hasIndices{false};
};

class Mesh
{
public:
	std::string name{"Mesh"};
	std::vector<Primitive> primitives;
};

class Model
{
public:
	struct Node
	{
		bool hasMesh{false};
		Mesh mesh;
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		glm::mat4 matrix; // Combination of rotation, translation and scale.
		std::vector<Node> children;
		Node* parent{ nullptr };
	};

public:
	Model();
	~Model();

	void cleanup();

	// Layout
	std::vector<Node> nodes;
	
	// Data
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	Buffer indexBuffer;
	Buffer vertexBuffer;
	Memory bufferMemory;

	bool hasImageMemory{false};
	bool hasMaterialMemory{ false };
	Memory imageMemory;
	Memory materialMemory;
};