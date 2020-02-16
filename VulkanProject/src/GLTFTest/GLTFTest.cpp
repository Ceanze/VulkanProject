#include "jaspch.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFTest.h"

#include "Vulkan/Instance.h"
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

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

	// Pipeline
	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(this->swapChain.getExtent(), &this->renderPass);
	PipelineInfo pipInfo;
	// Vertex input info
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

void GLTFTest::run()
{
	float dt = 0.0f;
	while (running && this->window.isOpen())
	{
		glfwPollEvents();

		auto startTime = std::chrono::high_resolution_clock::now();

		// Update view matrix
		glm::mat4 newView = getViewFromCamera(dt);
		this->memory.directTransfer(&this->bufferUniform, (void*)&newView[0][0], (uint32_t)sizeof(newView), (Offset)offsetof(UboData, view));
		vkDeviceWaitIdle(Instance::get().getDevice()); // This is bad!!

		// Render
		this->frame.beginFrame();
		this->frame.submit(Instance::get().getGraphicsQueue(), this->cmdBuffs);
		this->frame.endFrame();

		auto endTime = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration<float>(endTime - startTime).count();
	}
}

void GLTFTest::cleanup()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	this->depthTexture.cleanup();
	this->imageMemory.cleanup();

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
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(this->swapChain.getNumImages());
}

void GLTFTest::setupPostTEMP()
{
	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	//const std::string filePath = "..\\assets\\Models\\FlightHelmet\\FlightHelmet.gltf";
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
	JAS_INFO("Construct vertex data");
	std::vector<Vertex> vertices(positionsAccessor.count);
	for (size_t i = 0; i < positionsAccessor.count; i++)
	{
		size_t offset = i * sizeof(float) * 3;
		memcpy(&vertices[i].pos[0], positions + offset, sizeof(float) * 3);
		offset = i * sizeof(float) * 2;
		memcpy(&vertices[i].uv[0], texCoords + offset, sizeof(float) * 2);
	}

	// Set uniform data.
	UboData uboData;
	uboData.proj = glm::perspective(glm::radians(45.0f), (float)this->window.getWidth() / (float)this->window.getHeight(), 1.0f, 150.0f);
	uboData.view = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 10.f }, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.f, -1.f, 0.f });
	uboData.world = glm::mat4(1.0f);
	uint32_t unsiformBufferSize = sizeof(UboData);

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
	// TODO: Vertex and index buffers should use staging and be sent to the gpu by the command buffer instead!
	this->memory.directTransfer(&this->bufferVert, (void*)vertices.data(), (uint32_t)vertSize, 0);
	this->memory.directTransfer(&this->bufferInd, (void*)indices, (uint32_t)indSize, 0);

	this->memory.directTransfer(&this->bufferUniform, (void*)&uboData, unsiformBufferSize, 0);

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
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);
		value.depthStencil = { 1.0f, 0 };
		clearValues.push_back(value);
		this->cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, this->framebuffers[i].getFramebuffer(), this->swapChain.getExtent(), clearValues);
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

	loadTextures(this->model);
	loadMaterials(this->model);
	loadScenes(this->model);
}

void GLTFTest::loadTextures(tinygltf::Model& model)
{
	// TODO: Load all textures into memory and index them when needed.
	JAS_INFO("Textures:");
	if (model.textures.empty()) JAS_INFO(" ->No textures");
	for (size_t textureIndex = 0; textureIndex < model.textures.size(); textureIndex++)
	{
		tinygltf::Texture& texture = model.textures[textureIndex];
		tinygltf::Image& image = model.images[texture.source];
		JAS_INFO(" ->[{0}] name: {1} uri: {2}", textureIndex, texture.name.c_str(), image.uri.c_str());
	}

	// TODO: Load all samplers into memory and index them when needed.
	JAS_INFO("Samplers:");
	if (model.samplers.empty()) JAS_INFO(" ->No samplers");
	for (size_t samplersIndex = 0; samplersIndex < model.samplers.size(); samplersIndex++)
	{
		tinygltf::Sampler& samplers = model.samplers[samplersIndex];
		JAS_INFO(" ->[{0}] name: {1}", samplersIndex, samplers.name.c_str());
	}
}

void GLTFTest::loadMaterials(tinygltf::Model& model)
{
	// TODO: Load all materials into memory and index them when needed.
	JAS_INFO("Materials:");
	if (model.materials.empty()) JAS_INFO(" ->No materials");
	for (size_t materialIndex = 0; materialIndex < model.materials.size(); materialIndex++)
	{
		tinygltf::Material& material = model.materials[materialIndex];
		JAS_INFO(" ->[{0}] name: {1}", materialIndex, material.name.c_str());
	}
}

void GLTFTest::loadScenes(tinygltf::Model& model)
{
	for (auto& scene : model.scenes)
	{
		JAS_INFO("Scene: {0}", scene.name.c_str());
		// Load each node in the scene
		for (size_t nodeIndex = 0; nodeIndex < scene.nodes.size(); nodeIndex++)
		{
			tinygltf::Node& node = model.nodes[scene.nodes[nodeIndex]];
			loadNode(node, " ");
		}
	}
}

void GLTFTest::loadNode(tinygltf::Node& node, std::string indents)
{
	//TODO: Load attribute buffers and index buffer into memory for use.
	JAS_INFO("{0}->Node [{1}]", indents.c_str(), node.name.c_str());

	if (!node.scale.empty())
		JAS_INFO("{0} ->Has scale: ({1}, {2}, {3})", indents.c_str(), node.scale[0], node.scale[1], node.scale[2]);
	if (!node.translation.empty())
		JAS_INFO("{0} ->Has translation: ({1}, {2}, {3})", indents.c_str(), node.translation[0], node.translation[1], node.translation[2]);
	if (!node.rotation.empty())
		JAS_INFO("{0} ->Has rotation: ({1}, {2}, {3}, {4})", indents.c_str(), node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
	if (!node.matrix.empty())
		JAS_INFO("{0} ->Has matrix!", indents.c_str());

	// Check if it has a mesh
	if (node.mesh != -1)
	{
		tinygltf::Mesh& mesh = model.meshes[node.mesh];
		tinygltf::Primitive& primitive = mesh.primitives[0]; // Take first primitive. This holds the attributes, material, indices and the topology type (mode)
		JAS_ASSERT(primitive.mode == TINYGLTF_MODE_TRIANGLES, "Only support topology type TRIANGLE_LIST!");

		if (primitive.material != -1)
		{
			JAS_WARN("{0}  ->Has material!", indents.c_str());
		}

		// Check if it has indices
		if (primitive.indices != -1)
		{
			JAS_WARN("{0}  ->Has indices!", indents.c_str());
			tinygltf::Accessor& accessor = model.accessors[primitive.indices];
			tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			unsigned char* indicesData = &buffer.data.at(0) + bufferView.byteOffset;
			size_t compSize = 0;
			switch (accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:				compSize = sizeof(int8_t);		JAS_INFO("{0}   ->Component type: BYTE", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:		compSize = sizeof(uint8_t);		JAS_INFO("{0}   ->Component type: UNSIGNED_BYTE", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:			compSize = sizeof(double);		JAS_INFO("{0}   ->Component type: DOUBLE", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:				compSize = sizeof(float);		JAS_INFO("{0}   ->Component type: FLOAT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_INT:				compSize = sizeof(int32_t);		JAS_INFO("{0}   ->Component type: INT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:		compSize = sizeof(uint32_t);	JAS_INFO("{0}   ->Component type: UNSIGNED_INT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:				compSize = sizeof(int16_t);		JAS_INFO("{0}   ->Component type: SHORT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:	compSize = sizeof(uint16_t);	JAS_INFO("{0}   ->Component type: UNSIGNED_SHORT", indents.c_str()); break;
			}
			JAS_INFO("{0}   ->Component type size: {1} bytes", indents.c_str(), compSize);
			JAS_INFO("{0}   ->Type: 0x{1:b}", indents.c_str(), accessor.type);
			JAS_INFO("{0}   ->Count: {1}", indents.c_str(), accessor.count);
		}

		JAS_INFO("{0}  ->Attributes", indents.c_str());
		for (auto& attrib : primitive.attributes)
		{
			JAS_WARN("{0}   ->Attribute: {1}", indents.c_str(), attrib.first.c_str());
			tinygltf::Accessor& accessor = model.accessors[attrib.second];
			tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			unsigned char* attribData = &buffer.data.at(0) + bufferView.byteOffset;
			size_t compSize = 0;
			switch (accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:				compSize = sizeof(int8_t);		JAS_INFO("{0}    ->Component type: BYTE", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:		compSize = sizeof(uint8_t);		JAS_INFO("{0}    ->Component type: UNSIGNED_BYTE", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:			compSize = sizeof(double);		JAS_INFO("{0}    ->Component type: DOUBLE", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:				compSize = sizeof(float);		JAS_INFO("{0}    ->Component type: FLOAT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_INT:				compSize = sizeof(int32_t);		JAS_INFO("{0}    ->Component type: INT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:		compSize = sizeof(uint32_t);	JAS_INFO("{0}    ->Component type: UNSIGNED_INT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:				compSize = sizeof(int16_t);		JAS_INFO("{0}    ->Component type: SHORT", indents.c_str()); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:	compSize = sizeof(uint16_t);	JAS_INFO("{0}    ->Component type: UNSIGNED_SHORT", indents.c_str()); break;
			}
			JAS_INFO("{0}    ->Component type size: {1} bytes", indents.c_str(), compSize);
			JAS_INFO("{0}    ->Type: 0x{1:b}", indents.c_str(), accessor.type);
			JAS_INFO("{0}    ->Count: {1}", indents.c_str(), accessor.count);
		}
	}

	if(!node.children.empty())
		JAS_INFO("{0} ->Children:", indents.c_str());
	for (size_t childIndex = 0; childIndex < node.children.size(); childIndex++)
	{
		tinygltf::Node& child = model.nodes[node.children[childIndex]];
		loadNode(child, indents+"  ");
	}
}

glm::mat4 GLTFTest::getViewFromCamera(float dt)
{
	auto isKeyPressed = [&](int key)->bool {
		return glfwGetKey(this->window.getNativeWindow(), key) == GLFW_PRESS;
	};

	static glm::vec3 pos(0.0f, 0.0f, -10.0f);
	static glm::vec3 dir(0.0f, 0.0f, 1.0f);
	static glm::vec3 up(0.0f, -1.0f, 0.0f);
	static glm::vec3 GLOBAL_UP(0.0f, -1.0f, 0.0f);

	const float DEFAULT_SPEED = 8.0f;
	const float SPEED_FACTOR = 2.0f;
	const float DEFAUTL_ROT_SPEED = 3.1415f / 180.f * 90.0f; // 90 degrees per second.
	float speed = DEFAULT_SPEED * dt;
	float rotSpeed = DEFAUTL_ROT_SPEED * dt;

	if (isKeyPressed(GLFW_KEY_LEFT_SHIFT))
		speed = DEFAULT_SPEED * SPEED_FACTOR * dt;

	if (isKeyPressed(GLFW_KEY_A))
	{
		glm::vec3 right = glm::normalize(glm::cross(up, dir));
		pos = pos - right * speed;
	}
	if (isKeyPressed(GLFW_KEY_D))
	{
		glm::vec3 right = glm::normalize(glm::cross(up, dir));
		pos = pos + right * speed;
	}
	if (isKeyPressed(GLFW_KEY_W))
	{
		pos = pos + dir * speed;
	}
	if (isKeyPressed(GLFW_KEY_S))
	{
		pos = pos - dir * speed;
	}

	if (isKeyPressed(GLFW_KEY_LEFT))
	{
		glm::vec3 right = glm::normalize(glm::cross(up, dir));
		right = glm::rotate(right, -rotSpeed, GLOBAL_UP);
		if (glm::length(up - GLOBAL_UP) > 0.0001f)
			up = glm::rotate(up, -rotSpeed, GLOBAL_UP);
		up = glm::normalize(up);
		dir = glm::normalize(glm::cross(right, up));
	}
	if (isKeyPressed(GLFW_KEY_RIGHT))
	{
		glm::vec3 right = glm::normalize(glm::cross(up, dir));
		right = glm::rotate(right, rotSpeed, GLOBAL_UP);
		if (glm::length(up - GLOBAL_UP) > 0.0001f)
			up = glm::rotate(up, rotSpeed, GLOBAL_UP);
		up = glm::normalize(up);
		dir = glm::normalize(glm::cross(right, up));
	}
	if (isKeyPressed(GLFW_KEY_UP))
	{
		glm::vec3 right = glm::normalize(glm::cross(up, dir));
		up = glm::rotate(up, rotSpeed, right);
		up = glm::normalize(up);
		dir = glm::normalize(glm::cross(right, up));
	}
	if (isKeyPressed(GLFW_KEY_DOWN))
	{
		glm::vec3 right = glm::normalize(glm::cross(up, dir));
		up = glm::rotate(up, -rotSpeed, right);
		up = glm::normalize(up);
		dir = glm::normalize(glm::cross(right, up));
	}

	return glm::lookAt(pos, pos + dir, up);
}
