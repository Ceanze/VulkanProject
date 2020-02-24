#include "jaspch.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFLoader.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Pipeline/Pipeline.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/CommandBuffer.h"

#include "Vulkan/Pipeline/DescriptorManager.h"

#include "Vulkan/Instance.h"

#include <glm/gtc/type_ptr.hpp>

Model GLTFLoader::model = Model();
tinygltf::TinyGLTF GLTFLoader::loader = tinygltf::TinyGLTF();

void GLTFLoader::load(const std::string& filePath, Model* model)
{
	loadModel(*model, filePath);
}

void GLTFLoader::loadToStagingBuffer(const std::string& filePath, Model* model, Buffer* stagingBuff, Memory* stagingMemory)
{
	loadModel(*model, filePath, stagingBuff, stagingMemory);
}

void GLTFLoader::transferToModel(CommandPool* transferCommandPool, Model* model, Buffer* stagingBuff, Memory* stagingMemory)
{
	// Create memory and buffers.
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	uint32_t indicesSize = (uint32_t)(model->indices.size() * sizeof(uint32_t));
	if (indicesSize > 0)
	{
		model->indexBuffer.init(indicesSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueIndices);
		model->bufferMemory.bindBuffer(&model->indexBuffer);
	}

	uint32_t verticesSize = (uint32_t)(model->vertices.size() * sizeof(Vertex));
	model->vertexBuffer.init(verticesSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
	model->bufferMemory.bindBuffer(&model->vertexBuffer);

	// Create memory with the binded buffers
	model->bufferMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Copy data to the new buffers.
	CommandBuffer* cbuff = transferCommandPool->beginSingleTimeCommand();

	// Copy indices data.
	if (indicesSize > 0)
	{
		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = indicesSize;
		cbuff->cmdCopyBuffer(stagingBuff->getBuffer(), model->indexBuffer.getBuffer(), 1, &region);
	}

	// Copy vertex data.
	VkBufferCopy region = {};
	region.srcOffset = indicesSize;
	region.dstOffset = 0;
	region.size = verticesSize;
	cbuff->cmdCopyBuffer(stagingBuff->getBuffer(), model->vertexBuffer.getBuffer(), 1, &region);

	transferCommandPool->endSingleTimeCommand(cbuff);
}

void GLTFLoader::recordDraw(Model* model, CommandBuffer* commandBuffer, Pipeline* pipeline, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets)
{
	// TODO: Use different materials, can still use same pipeline if all meshes uses same type of material (i.e. PBR)!
	
	//VkBuffer vertexBuffers[] = { this->model.vertexBuffer.getBuffer() };
	//VkDeviceSize vertOffsets[] = { 0 };
	//this->cmdBuffs[i]->cmdBindVertexBuffers(0, 1, vertexBuffers, vertOffsets);
	if (model->indices.empty() == false)
		commandBuffer->cmdBindIndexBuffer(model->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	commandBuffer->cmdBindDescriptorSets(pipeline, 0, sets, offsets);
	for (Model::Node& node : model->nodes)
		drawNode(commandBuffer, node);
}

void GLTFLoader::loadModel(Model& model, const std::string& filePath)
{
	tinygltf::Model gltfModel;
	bool ret = false;
	size_t pos = filePath.rfind('.');
	if (pos != std::string::npos)
	{
		std::string err;
		std::string warn;

		std::string prefix = filePath.substr(pos);
		if(prefix == ".gltf")
			ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath);
		else if (prefix == ".glb")
			ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filePath); // for binary glTF(.glb)

		if (!warn.empty()) JAS_WARN("GLTF Waring: {0}", warn.c_str());
		if (!err.empty()) JAS_ERROR("GLTF Error: {0}", err.c_str());
	}
	
	JAS_ASSERT(ret, "Failed to parse glTF\n");

	loadTextures(model, gltfModel);
	loadMaterials(model, gltfModel);
	loadScenes(model, gltfModel);
}

void GLTFLoader::loadTextures(Model& model, tinygltf::Model& gltfModel)
{
	// TODO: Load all textures into memory and index them when needed.
	//JAS_INFO("Textures:");
	//if (gltfModel.textures.empty()) JAS_INFO(" ->No textures");
	for (size_t textureIndex = 0; textureIndex < gltfModel.textures.size(); textureIndex++)
	{
		tinygltf::Texture& texture = gltfModel.textures[textureIndex];
		tinygltf::Image& image = gltfModel.images[texture.source];
		//JAS_INFO(" ->[{0}] name: {1} uri: {2}", textureIndex, texture.name.c_str(), image.uri.c_str());
	}

	// TODO: Load all samplers into memory and index them when needed.
	//JAS_INFO("Samplers:");
	//if (gltfModel.samplers.empty()) JAS_INFO(" ->No samplers");
	for (size_t samplersIndex = 0; samplersIndex < gltfModel.samplers.size(); samplersIndex++)
	{
		tinygltf::Sampler& samplers = gltfModel.samplers[samplersIndex];
		//JAS_INFO(" ->[{0}] name: {1}", samplersIndex, samplers.name.c_str());
	}
}

void GLTFLoader::loadMaterials(Model& model, tinygltf::Model& gltfModel)
{
	// TODO: Load all materials into memory and index them when needed.
	//JAS_INFO("Materials:");
	//if (gltfModel.materials.empty()) JAS_INFO(" ->No materials");
	for (size_t materialIndex = 0; materialIndex < gltfModel.materials.size(); materialIndex++)
	{
		tinygltf::Material& material = gltfModel.materials[materialIndex];
		//JAS_INFO(" ->[{0}] name: {1}", materialIndex, material.name.c_str());
	}
}

void GLTFLoader::loadScenes(Model& model, tinygltf::Model& gltfModel)
{
	for (auto& scene : gltfModel.scenes)
	{
		//JAS_INFO("Scene: {0}", scene.name.c_str());
		// Load each node in the scene
		model.nodes.resize(scene.nodes.size());
		for (size_t nodeIndex = 0; nodeIndex < scene.nodes.size(); nodeIndex++)
		{
			tinygltf::Node& node = gltfModel.nodes[scene.nodes[nodeIndex]];
			loadNode(model, &model.nodes[nodeIndex], gltfModel, node, " ");
		}
	}

	// Create vertex and index buffers
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	uint32_t indicesSize = (uint32_t)(model.indices.size() * sizeof(uint32_t));
	if (indicesSize > 0)
	{
		model.indexBuffer.init(indicesSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueIndices);
		model.bufferMemory.bindBuffer(&model.indexBuffer);
	}

	uint32_t verticesSize = (uint32_t)(model.vertices.size() * sizeof(Vertex));
	model.vertexBuffer.init(verticesSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
	model.bufferMemory.bindBuffer(&model.vertexBuffer);

	// Create memory with the binded buffers
	model.bufferMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Transfer data to buffers
	if (indicesSize > 0)
		model.bufferMemory.directTransfer(&model.indexBuffer, (const void*)model.indices.data(), indicesSize, 0);
	model.bufferMemory.directTransfer(&model.vertexBuffer, (const void*)model.vertices.data(), verticesSize, 0);
}

void GLTFLoader::loadNode(Model& model, Model::Node* node, tinygltf::Model& gltfModel, tinygltf::Node& gltfNode, std::string indents)
{
	//JAS_INFO("{0}->Node [{1}]", indents.c_str(), gltfNode.name.c_str());

	if (!gltfNode.scale.empty())
	{
		//JAS_INFO("{0} ->Has scale: ({1}, {2}, {3})", indents.c_str(), gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
		node->scale = glm::vec3((float)gltfNode.scale[0], (float)gltfNode.scale[1], (float)gltfNode.scale[2]);
	}
	if (!gltfNode.translation.empty())
	{
		//JAS_INFO("{0} ->Has translation: ({1}, {2}, {3})", indents.c_str(), gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
		node->translation = glm::vec3((float)gltfNode.translation[0], (float)gltfNode.translation[1], (float)gltfNode.translation[2]);
	}
	if (!gltfNode.rotation.empty())
	{
		//JAS_INFO("{0} ->Has rotation: ({1}, {2}, {3}, {4})", indents.c_str(), gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2], gltfNode.rotation[3]);
		node->rotation = glm::quat((float)gltfNode.rotation[0], (float)gltfNode.rotation[1], (float)gltfNode.rotation[2], (float)gltfNode.rotation[3]);
	}
	if (!gltfNode.matrix.empty())
	{
		//JAS_INFO("{0} ->Has matrix!", indents.c_str());
		//JAS_WARN("{0} ->Matrix NOT implemented!", indents.c_str());
	}

	//if (!gltfNode.children.empty())
	//	JAS_INFO("{0} ->Children:", indents.c_str());
	node->children.resize(gltfNode.children.size());
	for (size_t childIndex = 0; childIndex < gltfNode.children.size(); childIndex++)
	{
		tinygltf::Node& child = gltfModel.nodes[gltfNode.children[childIndex]];
		loadNode(model, &node->children[childIndex], gltfModel, child, indents + "  ");
	}

	// Check if it has a mesh
	node->hasMesh = gltfNode.mesh != -1;
	if (node->hasMesh)
	{
		tinygltf::Mesh gltfMesh = gltfModel.meshes[gltfNode.mesh];
		Mesh& mesh = node->mesh;
		mesh.name = gltfMesh.name;
		//JAS_INFO("{0} ->Has mesh: {1}", indents.c_str(), mesh.name.c_str());

		mesh.primitives.resize(gltfMesh.primitives.size());
		for (size_t i = 0; i < gltfMesh.primitives.size(); i++)
		{
			Primitive& primitive = mesh.primitives[i];
			primitive.firstIndex = static_cast<uint32_t>(model.indices.size());
			uint32_t vertexStart = static_cast<uint32_t>(model.vertices.size()); // Used for indices.

			tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[i];
			
			// Indicies
			primitive.hasIndices = gltfPrimitive.indices != -1;
			if(primitive.hasIndices)
			{
				//JAS_INFO("{0}  ->Has indices", indents.c_str());
				tinygltf::Accessor& indAccessor = gltfModel.accessors[gltfPrimitive.indices];
				tinygltf::BufferView& indBufferView = gltfModel.bufferViews[indAccessor.bufferView];
				tinygltf::Buffer& indBuffer = gltfModel.buffers[indBufferView.buffer];
				unsigned char* indicesData = &indBuffer.data.at(0) + indBufferView.byteOffset;
				size_t indByteStride = indAccessor.ByteStride(indBufferView);

				primitive.indexCount = static_cast<uint32_t>(indAccessor.count);
				const void* dataPtr = &indBuffer.data.at(0) + indBufferView.byteOffset;
				for (size_t c = 0; c < indAccessor.count; c++)
				{
					switch (indAccessor.componentType)
					{
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					{
						const uint8_t* data = static_cast<const uint8_t*>(dataPtr);
						model.indices.push_back(data[c] + vertexStart);
					}
					break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					{
						const uint16_t* data = static_cast<const uint16_t*>(dataPtr);
						model.indices.push_back(data[c] + vertexStart);
					}
					break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					{
						const uint32_t* data = static_cast<const uint32_t*>(dataPtr);
						model.indices.push_back(data[c] + vertexStart);
					}
					break;
					}
				}
			}

			// Begin to fetch attributes and construct the vertices.
			unsigned char* posData = nullptr; // Assume to be vec3
			unsigned char* norData = nullptr; // Assume to be vec3
			unsigned char* uv0Data = nullptr; // Assume to be vec2
			size_t posByteStride = 0;
			size_t norByteStride = 0;
			size_t uv0ByteStride = 0;

			auto& attributes = gltfPrimitive.attributes;

			// POSITION
			//JAS_ASSERT(attributes.find("POSITION") != attributes.end(), "Primitive need to have one POSITION attribute!");
			//JAS_INFO("{0}  ->Has POSITION", indents.c_str());
			tinygltf::Accessor& posAccessor = gltfModel.accessors[attributes["POSITION"]];
			tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
			tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];
			posData = &posBuffer.data.at(0) + posBufferView.byteOffset;
			primitive.vertexCount = static_cast<uint32_t>(posAccessor.count);
			posByteStride = posAccessor.ByteStride(posBufferView);

			// NORMAL
			if (attributes.find("NORMAL") != attributes.end())
			{
				//JAS_INFO("{0}  ->Has NORMAL", indents.c_str());
				tinygltf::Accessor& accessor = gltfModel.accessors[attributes["NORMAL"]];
				tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
				norData = &buffer.data.at(0) + bufferView.byteOffset;
				norByteStride = accessor.ByteStride(bufferView);
			}

			// TEXCOORD_0
			if (attributes.find("TEXCOORD_0") != attributes.end())
			{
				//JAS_INFO("{0}  ->Has TEXCOORD_0", indents.c_str());
				tinygltf::Accessor& accessor = gltfModel.accessors[attributes["TEXCOORD_0"]];
				tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
				uv0Data = &buffer.data.at(0) + bufferView.byteOffset;
				uv0ByteStride = accessor.ByteStride(bufferView);
			}

			// Construct vertex data
			for (size_t v = 0; v < posAccessor.count; v++)
			{
				Vertex vert;
				vert.pos = glm::make_vec3((const float*)(posData + v*posByteStride));
				vert.nor = norData ? glm::normalize(glm::make_vec3((const float*)(norData + v*norByteStride))) : glm::vec3(0.0f);
				vert.uv0 = uv0Data ? glm::make_vec2((const float*)(uv0Data + v*uv0ByteStride)) : glm::vec2(0.0f);
				model.vertices.push_back(vert);
			}
		}
	}
}

void GLTFLoader::drawNode(CommandBuffer* commandBuffer, Model::Node& node)
{
	if (node.hasMesh)
	{
		Mesh& mesh = node.mesh;
		for (Primitive& primitive : mesh.primitives)
		{
			// TODO: Send node transformation as push constant per draw!

			if (primitive.hasIndices)
				commandBuffer->cmdDrawIndexed(primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			else
				commandBuffer->cmdDraw(primitive.vertexCount, 1, 0, 0);
		}
	}
	for (Model::Node& child : node.children)
		drawNode(commandBuffer, child);
}

void GLTFLoader::loadModel(Model& model, const std::string& filePath, Buffer* stagingBuff, Memory* stagingMemory)
{
	tinygltf::Model gltfModel;
	bool ret = false;
	size_t pos = filePath.rfind('.');
	if (pos != std::string::npos)
	{
		std::string err;
		std::string warn;

		std::string prefix = filePath.substr(pos);
		if (prefix == ".gltf")
			ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath);
		else if (prefix == ".glb")
			ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filePath); // for binary glTF(.glb)

		if (!warn.empty()) JAS_WARN("GLTF Waring: {0}", warn.c_str());
		if (!err.empty()) JAS_ERROR("GLTF Error: {0}", err.c_str());
	}

	JAS_ASSERT(ret, "Failed to parse glTF\n");

	loadTextures(model, gltfModel);
	loadMaterials(model, gltfModel);
	loadScenes(model, gltfModel, stagingBuff, stagingMemory);
}

void GLTFLoader::loadScenes(Model& model, tinygltf::Model& gltfModel, Buffer* stagingBuff, Memory* stagingMemory)
{
	for (auto& scene : gltfModel.scenes)
	{
		//JAS_INFO("Scene: {0}", scene.name.c_str());
		// Load each node in the scene
		model.nodes.resize(scene.nodes.size());
		for (size_t nodeIndex = 0; nodeIndex < scene.nodes.size(); nodeIndex++)
		{
			tinygltf::Node& node = gltfModel.nodes[scene.nodes[nodeIndex]];
			loadNode(model, &model.nodes[nodeIndex], gltfModel, node, " ");
		}
	}

	// Create vertex and index buffers
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_TRANSFER_BIT, Instance::get().getPhysicalDevice()) };
	uint32_t indicesSize = (uint32_t)(model.indices.size() * sizeof(uint32_t));
	uint32_t verticesSize = (uint32_t)(model.vertices.size() * sizeof(Vertex));

	stagingBuff->init(verticesSize + indicesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueIndices);
	stagingMemory->bindBuffer(&model.vertexBuffer);

	// Create memory with the binded buffers
	stagingMemory->init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Transfer data to buffers
	if (indicesSize > 0)
		stagingMemory->directTransfer(stagingBuff, (const void*)model.indices.data(), indicesSize, 0);
	stagingMemory->directTransfer(stagingBuff, (const void*)model.vertices.data(), verticesSize, (Offset)indicesSize);
}
