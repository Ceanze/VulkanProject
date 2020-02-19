#pragma once

// ------------ TEMP ------------
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "glTF/tiny_gltf.h"

#include "Vulkan/Buffers/Buffer.h"
#include "Vulkan/Buffers/Memory.h"
#include "Vulkan/Texture.h"
#include "Model/Model.h"

class CommandBuffer;
class Pipeline;

class GLTFLoader
{
public:
	GLTFLoader();
	~GLTFLoader();

	// Will read from file and load the data into the model pointer.
	void load(const std::string& filePath, Model* model);
	void recordDraw(Model* model, CommandBuffer* commandBuffer, Pipeline* pipeline, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets);

private:
	void loadModel(Model& model, const std::string& filePath);
	void loadTextures(Model& model, tinygltf::Model& gltfModel);
	void loadMaterials(Model& model, tinygltf::Model& gltfModel);
	void loadScenes(Model& model, tinygltf::Model& gltfModel);
	void loadNode(Model& model, Model::Node* node, tinygltf::Model& gltfModel, tinygltf::Node& gltfNode, std::string indents);

	void drawNode(CommandBuffer* commandBuffer, Model::Node& node);

private:
	Model model;
	tinygltf::TinyGLTF loader;
};