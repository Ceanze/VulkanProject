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
	// Will read from file and load the data into the model pointer.
	static void load(const std::string& filePath, Model* model);
	static void recordDraw(Model* model, CommandBuffer* commandBuffer, Pipeline* pipeline, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets);

	static void loadToStagingBuffer(const std::string& filePath, Model* model, Buffer* stagingBuff, Memory* stagingMemory);
	static void transferToModel(CommandPool* transferCommandPool, Model* model, Buffer* stagingBuff, Memory* stagingMemory);

private:
	static void loadModel(Model& model, const std::string& filePath);
	static void loadTextures(Model& model, tinygltf::Model& gltfModel);
	static void loadMaterials(Model& model, tinygltf::Model& gltfModel);
	static void loadScenes(Model& model, tinygltf::Model& gltfModel);
	static void loadNode(Model& model, Model::Node* node, tinygltf::Model& gltfModel, tinygltf::Node& gltfNode, std::string indents);

	static void drawNode(CommandBuffer* commandBuffer, Model::Node& node);

	static void loadModel(Model& model, const std::string& filePath, Buffer* stagingBuff, Memory* stagingMemory);
	static void loadScenes(Model& model, tinygltf::Model& gltfModel, Buffer* stagingBuff, Memory* stagingMemory);

private:
	static Model model;
	static tinygltf::TinyGLTF loader;
};