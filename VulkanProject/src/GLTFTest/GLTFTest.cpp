#include "jaspch.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFTest.h"

#include "Vulkan/Instance.h"
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

GLTFTest::GLTFTest()
{
}

GLTFTest::~GLTFTest()
{
}

void GLTFTest::init()
{
	Logger::init();

	this->window.init(1280, 720, "Vulkan Project");

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());

	this->shader.addStage(Shader::Type::VERTEX, "gltfTestVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "gltfTestFrag.spv");
	this->shader.init();

	// Add attachments
	this->renderPass.addDefaultColorAttachment(this->swapChain.getImageFormat());

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 };
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

	setupPreTEMP();
	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(this->swapChain.getExtent(), &this->renderPass);
	PipelineInfo pipInfo;
	pipInfo.vertexInputInfo = {};
	pipInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	pipInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipInfo.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	pipInfo.vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	pipInfo.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	this->pipeline.setPipelineInfo(PipelineInfoFlag::VERTEX_INPUT, pipInfo);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	JAS_INFO("Created Renderer!");

	this->commandPool.init(CommandPool::Queue::GRAPHICS);
	JAS_INFO("Created Command Pool!");

	this->framebuffers.resize(this->swapChain.getNumImages());

	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->swapChain.getImageViews()[i]);
		this->framebuffers[i].init(this->swapChain.getNumImages(), &this->renderPass, imageViews, this->swapChain.getExtent());
	}

	this->frame.init(&this->swapChain);

	setupPostTEMP();
}

void GLTFTest::run()
{
	while (running && this->window.isOpen())
	{
		glfwPollEvents();

		this->frame.beginFrame();
		this->frame.submit(Instance::get().getGraphicsQueue(), this->cmdBuffs);
		this->frame.endFrame();
	}
}

void GLTFTest::cleanup()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	this->bufferUniform.cleanup();
	this->bufferVert.cleanup();
	this->bufferInd.cleanup();
	this->memory.cleanup();
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

void GLTFTest::setupPreTEMP()
{
	//this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Positions
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(this->swapChain.getNumImages());
}

void GLTFTest::setupPostTEMP()
{
	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	loadModel(filePath);
	JAS_INFO("Loaded model");

	JAS_INFO("Processing data...");
	tinygltf::Scene& scene = this->model.scenes[0]; // nodes: 0
	JAS_INFO("Scene 0");

	tinygltf::Node& node = this->model.nodes[scene.nodes[0]]; // Name: Cube, Mesh: 0
	JAS_INFO("->Node: ", node.name.c_str());

	tinygltf::Mesh& mesh = this->model.meshes[node.mesh];
	JAS_INFO("	->Mesh: ", node.name.c_str());
	tinygltf::Primitive& primitive = mesh.primitives[0]; // Take first primitive. This holds the attributes, material, indices and the topology type (mode)
	JAS_ASSERT(primitive.mode == TINYGLTF_MODE_TRIANGLES, "Only support topology type TRIANGLE_LIST!");
	JAS_INFO("	  mode: TRIANGLES");

	// Indices (Assume it uses indices)
	tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
	tinygltf::BufferView& indicesView = model.bufferViews[indicesAccessor.bufferView];
	tinygltf::Buffer& indicesBuffer = model.buffers[indicesView.buffer];
	JAS_INFO("	  ->Fetched indices");
	unsigned char* indices = &indicesBuffer.data.at(0) + indicesView.byteOffset;

	// Positions
	tinygltf::Accessor& positionsAccessor = model.accessors[primitive.attributes["POSITION"]];
	tinygltf::BufferView& positionsView = model.bufferViews[positionsAccessor.bufferView];
	tinygltf::Buffer& positionsBuffer = model.buffers[positionsView.buffer];
	JAS_INFO("	  ->Fetched positions");
	unsigned char* positions = &positionsBuffer.data.at(0) + positionsView.byteOffset;

	// Texture coordinates
	tinygltf::Accessor& texCoordsAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
	tinygltf::BufferView& texCoordsView = model.bufferViews[texCoordsAccessor.bufferView];
	tinygltf::Buffer& texCoordsBuffer = model.buffers[texCoordsView.buffer];
	JAS_INFO("	  ->Fetched texCoords");
	unsigned char* texCoords = &texCoordsBuffer.data.at(0) + texCoordsView.byteOffset;
	
	JAS_ASSERT(indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, "Indices component type need to be unsigned short!");
	size_t indSize = indicesAccessor.count * sizeof(unsigned short); // Should be indicesView.byteLength
	size_t vertSize = positionsAccessor.count * sizeof(Vertex);

	// Fill vertex data.
	std::vector<Vertex> vertices(positionsAccessor.count);
	for (size_t i = 0; i < positionsAccessor.count; i++)
	{
		size_t offset = i * sizeof(float) * 3;
		memcpy(&vertices[i].pos[0], positions + offset, sizeof(float) * 3);
		offset = i * sizeof(float) * 2;
		memcpy(&vertices[i].uv[0], texCoords + offset, sizeof(float) * 2);
	}

	// Set uniform data.
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)this->window.getWidth() / (float)this->window.getHeight(), 1.0f, 150.0f);
	glm::mat4 view = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 10.f }, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.f, -1.f, 0.f });
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 mvp = proj * view * model;
	uint32_t unsiformBufferSize = sizeof(glm::mat4);

	// Create the buffer and memory
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->bufferVert.init(vertSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, queueIndices);
	this->bufferInd.init(indSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueIndices);
	this->bufferUniform.init(unsiformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	this->memory.bindBuffer(&this->bufferVert);
	this->memory.bindBuffer(&this->bufferInd);
	this->memory.bindBuffer(&this->bufferUniform);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Transfer the data to the buffers.
	this->memory.directTransfer(&this->bufferVert, (void*)vertices.data(), (uint32_t)vertSize, 0);
	this->memory.directTransfer(&this->bufferInd, (void*)indices, (uint32_t)indSize, 0);
	this->memory.directTransfer(&this->bufferUniform, (void*)&mvp[0][0], unsiformBufferSize, 0);

	// Update descriptor
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		this->descManager.updateBufferDesc(0, 0, this->bufferUniform.getBuffer(), 0, unsiformBufferSize);
		this->descManager.updateSets(i);
	}

	// Record command buffers
	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		this->cmdBuffs[i] = this->commandPool.createCommandBuffer();
		this->cmdBuffs[i]->begin(0);
		this->cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, this->framebuffers[i].getFramebuffer(), this->swapChain.getExtent(), { 0.0f, 0.0f, 0.0f, 1.0f });
		this->cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		VkBuffer vertexBuffers[] = { this->bufferVert.getBuffer() };
		VkDeviceSize vertOffsets[] = { 0 };
		this->cmdBuffs[i]->cmdBindVertexBuffers(0, 1, vertexBuffers, vertOffsets);
		this->cmdBuffs[i]->cmdBindIndexBuffer(this->bufferInd.getBuffer(), 0, VK_INDEX_TYPE_UINT16); // Unsinged short
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(i, 0) };
		std::vector<uint32_t> offsets;
		this->cmdBuffs[i]->cmdBindDescriptorSets(&this->pipeline, 0, sets, offsets);
		this->cmdBuffs[i]->cmdDrawIndexed(indicesAccessor.count, 1, 0, 0, 0);
		this->cmdBuffs[i]->cmdEndRenderPass();
		this->cmdBuffs[i]->end();
	}
}

void GLTFTest::loadModel(const std::string& filePath)
{
	bool ret = false;
	size_t pos = filePath.rfind('.');
	if (pos != std::string::npos)
	{
		std::string err;
		std::string warn;

		std::string prefix = filePath.substr(pos);
		if(prefix == ".gltf")
			ret = this->loader.LoadASCIIFromFile(&this->model, &err, &warn, filePath);
		else if (prefix == ".glb")
			ret = this->loader.LoadBinaryFromFile(&this->model, &err, &warn, filePath); // for binary glTF(.glb)

		if (!warn.empty()) JAS_WARN("GLTF Waring: {0}", warn.c_str());
		if (!err.empty()) JAS_ERROR("GLTF Error: {0}", err.c_str());
	}
	
	JAS_ASSERT(ret, "Failed to parse glTF\n");
}
