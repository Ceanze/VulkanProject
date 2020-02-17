#pragma once

#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Texture.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct VertexAttribs
{
	struct Attrib
	{
		VkFormat format;
		uint32_t offset;
		uint32_t stride;
		uint32_t location;
		uint32_t binding;
	};
	std::vector<Attrib> attribs;

	std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription > bindingDescriptions = {};
		for (Attrib& attrib : this->attribs)
		{
			VkVertexInputBindingDescription attribDesc;
			attribDesc.binding = attrib.binding;
			attribDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			attribDesc.stride = attrib.stride;
			bindingDescriptions.push_back(attribDesc);
		}
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
		for (Attrib& attrib : this->attribs)
		{
			VkVertexInputAttributeDescription attribDesc;
			attribDesc.binding = attrib.binding;
			attribDesc.location = attrib.location;
			attribDesc.format = attrib.format;
			attribDesc.offset = attrib.offset;
			attributeDescriptions.push_back(attribDesc);
		}
		return attributeDescriptions;
	}
};

class Mesh
{
public:
	std::string name{"Mesh"};
	bool hasIndices{false};
	uint32_t numIndices{0};
	Buffer indices;
	std::unordered_map<std::string, Buffer> attributes; // Map attribute type to buffer. Type can be POSITION, NORMAL, TEXCOORD_0, TANGENT, ...
	Memory bufferMemory; // Memory for position, normal, indices, texcoords ...
};

class Model
{
public:
	struct Node
	{
		Mesh* mesh{ nullptr };
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		std::vector<Node> children;
	};

public:
	Model();
	~Model();

	void cleanup();

	// Layout
	std::vector<Node> nodes;
	
	// Data
	std::vector<Mesh> meshes;
	bool hasImageMemory{false};
	bool hasMaterialMemory{ false };
	Memory imageMemory;
	Memory materialMemory;
};