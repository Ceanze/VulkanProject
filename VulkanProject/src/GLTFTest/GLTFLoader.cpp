#include "jaspch.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFLoader.h"

#include "Vulkan/Instance.h"
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

GLTFLoader::GLTFLoader()
{
}

GLTFLoader::~GLTFLoader()
{
}

void GLTFLoader::init()
{
	Logger::init();

	this->window.init(1280, 720, "Vulkan Project");
	this->camera = new Camera(this->window.getAspectRatio(), 45.f, { 0.f, 0.f, 10.f }, { 0.f, 0.f, 0.f }, 0.8f);

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());

	this->shader.addStage(Shader::Type::VERTEX, "gltfTestVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "gltfTestFrag.spv");
	this->shader.init();

	// Add attachments
	this->renderPass.addDefaultColorAttachment(this->swapChain.getImageFormat());
	this->renderPass.addDefaultDepthAttachment();

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 }; // One color attachment
	subpassInfo.depthStencilAttachmentIndex = 1; // Depth attachment
	this->renderPass.addSubpass(subpassInfo);

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	this->renderPass.addSubpassDependency(subpassDependency);
	this->renderPass.init();

	this->commandPool.init(CommandPool::Queue::GRAPHICS);
	JAS_INFO("Created Command Pool!");

	setupPreTEMP();

	// Depth texture
	VkFormat depthFormat = findDepthFormat(Instance::get().getPhysicalDevice());
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->depthTexture.init(this->swapChain.getExtent().width, this->swapChain.getExtent().height,
		depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, queueIndices);

	// Bind image to memory.
	this->imageMemory.bindTexture(&this->depthTexture);
	this->imageMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Create image view for the depth texture.
	this->depthTexture.getImageView().init(this->depthTexture.getVkImage(), VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Transistion image
	Image::TransistionDesc desc;
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	desc.pool = &this->commandPool;
	this->depthTexture.getImage().transistionLayout(desc);

	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(this->swapChain.getExtent(), &this->renderPass);
	PipelineInfo pipInfo;
	// Vertex input info
	pipInfo.vertexInputInfo = {};
	pipInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	//auto bindingDescription = Vertex::getBindingDescriptions();
	//auto attributeDescriptions = Vertex::getAttributeDescriptions();
	pipInfo.vertexInputInfo.vertexBindingDescriptionCount = 0;
	pipInfo.vertexInputInfo.pVertexBindingDescriptions = nullptr;// &bindingDescription;
	pipInfo.vertexInputInfo.vertexAttributeDescriptionCount = 0;// static_cast<uint32_t>(attributeDescriptions.size());
	pipInfo.vertexInputInfo.pVertexAttributeDescriptions = nullptr;// attributeDescriptions.data();
	this->pipeline.setPipelineInfo(PipelineInfoFlag::VERTEX_INPUT, pipInfo);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	JAS_INFO("Created Renderer!");

	this->framebuffers.resize(this->swapChain.getNumImages());

	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->swapChain.getImageViews()[i]);  // Add color attachment
		imageViews.push_back(this->depthTexture.getVkImageView()); // Add depth image view
		this->framebuffers[i].init(this->swapChain.getNumImages(), &this->renderPass, imageViews, this->swapChain.getExtent());
	}

	this->frame.init(&this->swapChain);

	setupPostTEMP();
}

void GLTFLoader::run()
{
	float dt = 0.0f;
	while (running && this->window.isOpen())
	{
		glfwPollEvents();

		if (glfwGetKey(this->window.getNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			running = false;

		auto startTime = std::chrono::high_resolution_clock::now();

		// Update view matrix
		this->memory.directTransfer(&this->bufferUniform, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), (Offset)offsetof(UboData, vp));

		// Render
		this->frame.beginFrame();
		this->frame.submit(Instance::get().getGraphicsQueue().queue, this->cmdBuffs);
		this->frame.endFrame();
		this->camera->update(dt);

		auto endTime = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration<float>(endTime - startTime).count();

		this->window.setTitle("Delta Time: " + std::to_string(dt * 1000.f) + " ms");
	}
}

void GLTFLoader::cleanup()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	this->depthTexture.cleanup();
	this->imageMemory.cleanup();

	this->bufferUniform.cleanup();
	this->memory.cleanup();

	this->model.cleanup();

	this->descManager.cleanup();

	this->frame.cleanup();
	this->commandPool.cleanup();
	for (auto& framebuffer : this->framebuffers)
		framebuffer.cleanup();
	this->pipeline.cleanup();
	this->renderPass.cleanup();
	this->shader.cleanup();
	this->swapChain.cleanup();
	Instance::get().cleanup();
	this->window.cleanup();
}

void GLTFLoader::setupPreTEMP()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(this->swapChain.getNumImages());

	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	loadModel(this->model, filePath);
}

void GLTFLoader::setupPostTEMP()
{
	// Set uniform data.
	UboData uboData;
	uboData.vp = this->camera->getMatrix();
	uboData.world = glm::mat4(1.0f);
	uboData.world[3][3] = 1.0f;
	uint32_t unsiformBufferSize = sizeof(UboData);

	// Create the buffer and memory
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->bufferUniform.init(unsiformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->memory.bindBuffer(&this->bufferUniform);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Transfer the data to the buffer.
	this->memory.directTransfer(&this->bufferUniform, (void*)& uboData, unsiformBufferSize, 0);

	// Update descriptor
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		VkDeviceSize vertexBufferSize = this->model.vertices.size() * sizeof(Vertex);
		this->descManager.updateBufferDesc(0, 0, this->model.vertexBuffer.getBuffer(), 0, vertexBufferSize);
		this->descManager.updateBufferDesc(0, 1, this->bufferUniform.getBuffer(), 0, unsiformBufferSize);
		this->descManager.updateSets(i);
	}

	// Record command buffers
	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		this->cmdBuffs[i] = this->commandPool.createCommandBuffer();
		this->cmdBuffs[i]->begin(0);
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);
		value.depthStencil = { 1.0f, 0 };
		clearValues.push_back(value);
		this->cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, this->framebuffers[i].getFramebuffer(), this->swapChain.getExtent(), clearValues);

		// TODO: Use different materials, can still use same pipeline if all meshes uses same type of material (i.e. PBR)!
		this->cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		//VkBuffer vertexBuffers[] = { this->model.vertexBuffer.getBuffer() };
		//VkDeviceSize vertOffsets[] = { 0 };
		//this->cmdBuffs[i]->cmdBindVertexBuffers(0, 1, vertexBuffers, vertOffsets);
		if(this->model.indices.empty() == false)
			this->cmdBuffs[i]->cmdBindIndexBuffer(this->model.indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(i, 0) };
		std::vector<uint32_t> offsets;
		this->cmdBuffs[i]->cmdBindDescriptorSets(&this->pipeline, 0, sets, offsets);
		for (Model::Node& node : this->model.nodes)
		{
			drawNode(i, node);
		}

		this->cmdBuffs[i]->cmdEndRenderPass();
		this->cmdBuffs[i]->end();
	}
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
			ret = this->loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath);
		else if (prefix == ".glb")
			ret = this->loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filePath); // for binary glTF(.glb)

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
	JAS_INFO("Textures:");
	if (gltfModel.textures.empty()) JAS_INFO(" ->No textures");
	for (size_t textureIndex = 0; textureIndex < gltfModel.textures.size(); textureIndex++)
	{
		tinygltf::Texture& texture = gltfModel.textures[textureIndex];
		tinygltf::Image& image = gltfModel.images[texture.source];
		JAS_INFO(" ->[{0}] name: {1} uri: {2}", textureIndex, texture.name.c_str(), image.uri.c_str());
	}

	// TODO: Load all samplers into memory and index them when needed.
	JAS_INFO("Samplers:");
	if (gltfModel.samplers.empty()) JAS_INFO(" ->No samplers");
	for (size_t samplersIndex = 0; samplersIndex < gltfModel.samplers.size(); samplersIndex++)
	{
		tinygltf::Sampler& samplers = gltfModel.samplers[samplersIndex];
		JAS_INFO(" ->[{0}] name: {1}", samplersIndex, samplers.name.c_str());
	}
}

void GLTFLoader::loadMaterials(Model& model, tinygltf::Model& gltfModel)
{
	// TODO: Load all materials into memory and index them when needed.
	JAS_INFO("Materials:");
	if (gltfModel.materials.empty()) JAS_INFO(" ->No materials");
	for (size_t materialIndex = 0; materialIndex < gltfModel.materials.size(); materialIndex++)
	{
		tinygltf::Material& material = gltfModel.materials[materialIndex];
		JAS_INFO(" ->[{0}] name: {1}", materialIndex, material.name.c_str());
	}
}

void GLTFLoader::loadScenes(Model& model, tinygltf::Model& gltfModel)
{
	for (auto& scene : gltfModel.scenes)
	{
		JAS_INFO("Scene: {0}", scene.name.c_str());
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
	JAS_INFO("{0}->Node [{1}]", indents.c_str(), gltfNode.name.c_str());

	if (!gltfNode.scale.empty())
	{
		JAS_INFO("{0} ->Has scale: ({1}, {2}, {3})", indents.c_str(), gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
		node->scale = glm::vec3((float)gltfNode.scale[0], (float)gltfNode.scale[1], (float)gltfNode.scale[2]);
	}
	if (!gltfNode.translation.empty())
	{
		JAS_INFO("{0} ->Has translation: ({1}, {2}, {3})", indents.c_str(), gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
		node->translation = glm::vec3((float)gltfNode.translation[0], (float)gltfNode.translation[1], (float)gltfNode.translation[2]);
	}
	if (!gltfNode.rotation.empty())
	{
		JAS_INFO("{0} ->Has rotation: ({1}, {2}, {3}, {4})", indents.c_str(), gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2], gltfNode.rotation[3]);
		node->rotation = glm::quat((float)gltfNode.rotation[0], (float)gltfNode.rotation[1], (float)gltfNode.rotation[2], (float)gltfNode.rotation[3]);
	}
	if (!gltfNode.matrix.empty())
	{
		JAS_INFO("{0} ->Has matrix!", indents.c_str());
		JAS_WARN("{0} ->Matrix NOT implemented!", indents.c_str());
	}

	if (!gltfNode.children.empty())
		JAS_INFO("{0} ->Children:", indents.c_str());
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
		JAS_INFO("{0} ->Has mesh: {1}", indents.c_str(), mesh.name.c_str());

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
				JAS_INFO("{0}  ->Has indices", indents.c_str());
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
			JAS_ASSERT(attributes.find("POSITION") != attributes.end(), "Primitive need to have one POSITION attribute!");
			JAS_INFO("{0}  ->Has POSITION", indents.c_str());
			tinygltf::Accessor& posAccessor = gltfModel.accessors[attributes["POSITION"]];
			tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
			tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];
			posData = &posBuffer.data.at(0) + posBufferView.byteOffset;
			primitive.vertexCount = posAccessor.count;
			posByteStride = posAccessor.ByteStride(posBufferView);

			// NORMAL
			if (attributes.find("NORMAL") != attributes.end())
			{
				JAS_INFO("{0}  ->Has NORMAL", indents.c_str());
				tinygltf::Accessor& accessor = gltfModel.accessors[attributes["NORMAL"]];
				tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
				norData = &buffer.data.at(0) + bufferView.byteOffset;
				norByteStride = accessor.ByteStride(bufferView);
			}

			// TEXCOORD_0
			if (attributes.find("TEXCOORD_0") != attributes.end())
			{
				JAS_INFO("{0}  ->Has TEXCOORD_0", indents.c_str());
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

void GLTFLoader::drawNode(int index, Model::Node& node)
{
	if (node.hasMesh)
	{
		Mesh& mesh = node.mesh;
		for (Primitive& primitive : mesh.primitives)
		{
			// TODO: Send node transformation as push constant per draw!

			if (primitive.hasIndices)
				this->cmdBuffs[index]->cmdDrawIndexed(primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			else
				this->cmdBuffs[index]->cmdDraw(primitive.vertexCount, 1, 0, 0);
		}
	}
	for (Model::Node& child : node.children)
		drawNode(index, child);
}