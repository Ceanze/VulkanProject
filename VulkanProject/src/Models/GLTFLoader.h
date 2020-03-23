#pragma once

// ------------ TEMP ------------
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "glTF/tiny_gltf.h"

#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Texture.h"
#include "Model/Model.h"

class CommandPool;
class CommandBuffer;
class Pipeline;

class GLTFLoader
{
public:
	struct StagingBuffers
	{
		Buffer geometryBuffer;
		Buffer imageBuffer;
		Memory geometryMemory;
		Memory imageMemory;

		void initMemory()
		{
			this->geometryMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			this->imageMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}

		void cleanup() 
		{
			this->geometryBuffer.cleanup();
			this->imageBuffer.cleanup();
			this->geometryMemory.cleanup();
			this->imageMemory.cleanup();
		}
	};

public:
	static void initDefaultData(CommandPool* transferCommandPool);
	static void cleanupDefaultData();

	// Will read from file and load the data into the model pointer.
	static void load(const std::string& filePath, Model* model);

	// TODO: This should be in a renderer!
	static void recordDraw(Model* model, CommandBuffer* commandBuffer, Pipeline* pipeline, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets);

	static void prepareStagingBuffer(const std::string& filePath, Model* model, StagingBuffers* stagingBuffers);
	static void transferToModel(CommandPool* transferCommandPool, Model* model, StagingBuffers* stagingBuffers);

private:
	static void loadModel(Model& model, const std::string& filePath);
	static void loadTextures(std::string& folderPath, Model& model, tinygltf::Model& gltfModel, StagingBuffers* stagingBuffers);
	static void loadImageData(std::string& folderPath, tinygltf::Image& image, tinygltf::Model& gltfModel, std::vector<uint8_t>& data);
	static void loadSamplerData(tinygltf::Sampler& samplerGltf, Sampler& sampler);
	static void loadMaterials(Model& model, tinygltf::Model& gltfModel);
	static void loadScenes(Model& model, tinygltf::Model& gltfModel);
	static void loadNode(Model& model, Model::Node* node, tinygltf::Model& gltfModel, tinygltf::Node& gltfNode, std::string indents);

	static void drawNode(Pipeline* pipeline, CommandBuffer* commandBuffer, Model::Node& node);

	static void loadModel(Model& model, const std::string& filePath, StagingBuffers* stagingBuffers);
	static void loadScenes(Model& model, tinygltf::Model& gltfModel, StagingBuffers* stagingBuffers);

private:
	static Model model;
	static tinygltf::TinyGLTF loader;

	struct DefaultData
	{
		Texture texture;
		Sampler sampler;
		Memory memory;
	};
	static DefaultData defaultData;
};