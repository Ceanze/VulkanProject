#pragma once

#include "Core/Window.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Pipeline/Shader.h"
#include "Vulkan/Pipeline/Pipeline.h"
#include "Vulkan/Pipeline/RenderPass.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Buffers/Framebuffer.h"
#include "Vulkan/Frame.h"

#include "Vulkan/Pipeline/DescriptorManager.h"

// ------------ TEMP ------------
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "glTF/tiny_gltf.h"

#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Texture.h"
#include "Model/Model.h"
#include "Core/Camera.h"

class GLTFLoader
{
public:
	GLTFLoader();
	~GLTFLoader();

	void init();
	void run();
	void cleanup();

private:
	struct UboData
	{
		glm::mat4 world;
		glm::mat4 vp;
	};

	void setupPreTEMP();
	void setupPostTEMP();

	void loadModel(Model& model, const std::string& filePath);
	void loadTextures(Model& model, tinygltf::Model& gltfModel);
	void loadMaterials(Model& model, tinygltf::Model& gltfModel);
	void loadScenes(Model& model, tinygltf::Model& gltfModel);
	void loadNode(Model& model, Model::Node* node, tinygltf::Model& gltfModel, tinygltf::Node& gltfNode, std::string indents);

	void drawNode(int index, Model::Node& node);

private:
	Window window;
	SwapChain swapChain;
	Shader shader;
	Pipeline pipeline;
	RenderPass renderPass;
	CommandPool commandPool;
	std::vector<Framebuffer> framebuffers;
	Frame frame;

	bool running = true;

	DescriptorLayout descLayout;
	DescriptorManager descManager;

	Camera* camera;

	// For loading model
	Model model;
	tinygltf::TinyGLTF loader;

	CommandBuffer* cmdBuffs[3];
	Buffer bufferUniform;
	Texture depthTexture;
	Memory memory; // Holds uniform data
	Memory imageMemory; // Holds depth texture
};