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
	auto bindingDescription = this->vertexAttribs.getBindingDescriptions();
	auto attributeDescriptions = this->vertexAttribs.getAttributeDescriptions();
	pipInfo.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
	pipInfo.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	pipInfo.vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
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

void GLTFLoader::cleanup()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	this->depthTexture.cleanup();
	this->imageMemory.cleanup();

	this->bufferUniform.cleanup();
	this->memory.cleanup();

	this->model.cleanup();
	if (this->model.meshes.empty())
	{
		this->bufferVert.cleanup();
		this->bufferInd.cleanup();
	}

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
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Uniforms
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(this->swapChain.getNumImages());

	//const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	const std::string filePath = "..\\assets\\Models\\FlightHelmet\\FlightHelmet.gltf";
	loadModel(this->model, filePath);
}

void GLTFLoader::setupPostTEMP()
{
	// Set uniform data.
	UboData uboData;
	uboData.proj = glm::perspective(glm::radians(45.0f), (float)this->window.getWidth() / (float)this->window.getHeight(), 1.0f, 150.0f);
	uboData.view = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 10.f }, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.f, -1.f, 0.f });
	uboData.world = glm::mat4(10.0f); // Scale by 10
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
	loadMeshes(model, gltfModel);
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

void GLTFLoader::loadMeshes(Model& model, tinygltf::Model& gltfModel)
{
	model.meshes.resize(gltfModel.meshes.size());

	JAS_INFO("Meshes:");
	if (gltfModel.meshes.empty()) JAS_INFO(" ->No meshes");
	for (size_t meshIndex = 0; meshIndex < gltfModel.meshes.size(); meshIndex++)
	{
		tinygltf::Mesh& gltfMesh = gltfModel.meshes[meshIndex];
		JAS_INFO(" ->Mesh: {0}", gltfMesh.name.c_str());

		Mesh& mesh = model.meshes[meshIndex];
		mesh.name = gltfMesh.name;

		tinygltf::Primitive& primitive = gltfMesh.primitives[0]; // Take first primitive. This holds the attributes, material, indices and the topology type (mode)
		JAS_ASSERT(primitive.mode == TINYGLTF_MODE_TRIANGLES, "Only support topology type TRIANGLE_LIST!");

		if (primitive.material != -1)
		{
			JAS_WARN("   ->Has material!");
		}

		// Create memory and buffers (Need to be done before fetching the data)
		size_t memSize = 0;
		std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
		if (primitive.indices != -1)
		{
			JAS_WARN("   ->Has indices!");
			mesh.hasIndices = true;
			tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
			size_t compSize = getComponentTypeSize(accessor.componentType);
			size_t numComps = getNumComponentsPerElement(accessor.type);
			size_t size = compSize * accessor.count * numComps;
			memSize += size;

			switch (accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:				JAS_INFO("    ->Component type: BYTE"); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:		JAS_INFO("    ->Component type: UNSIGNED_BYTE"); break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:			JAS_INFO("    ->Component type: DOUBLE"); break;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:				JAS_INFO("    ->Component type: FLOAT"); break;
			case TINYGLTF_COMPONENT_TYPE_INT:				JAS_INFO("    ->Component type: INT"); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:		JAS_INFO("    ->Component type: UNSIGNED_INT"); break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:				JAS_INFO("    ->Component type: SHORT"); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:	JAS_INFO("    ->Component type: UNSIGNED_SHORT"); break;
			}
			JAS_INFO("    ->Component type size: {0} bytes", compSize);
			// type can be: TINYGLTF_TYPE_SCALAR, TINYGLTF_TYPE_VECTOR, TINYGLTF_TYPE_MATRIX, TINYGLTF_TYPE_VEC2, TINYGLTF_TYPE_VEC3, TINYGLTF_TYPE_VEC4, TINYGLTF_TYPE_MAT2, TINYGLTF_TYPE_MAT3, TINYGLTF_TYPE_MAT4
			JAS_INFO("    ->Type: 0x{0:b} ({1})", accessor.type, numComps);
			JAS_INFO("    ->Count: {0}", accessor.count);

			// Create buffer and bind it to memory.
			mesh.indices.init(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, queueIndices);
			mesh.bufferMemory.bindBuffer(&mesh.indices);
			mesh.numIndices = accessor.count;
		}
		JAS_INFO("   ->Attributes");
		for (auto& attrib : primitive.attributes)
		{
			JAS_WARN("    ->Attribute: {0}", attrib.first.c_str());
			tinygltf::Accessor& accessor = gltfModel.accessors[attrib.second];
			size_t compSize = getComponentTypeSize(accessor.componentType);
			size_t numComps = getNumComponentsPerElement(accessor.type);
			size_t size = compSize * accessor.count * numComps;
			memSize += size;

			switch (accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:				JAS_INFO("    ->Component type: BYTE"); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:		JAS_INFO("    ->Component type: UNSIGNED_BYTE"); break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:			JAS_INFO("    ->Component type: DOUBLE"); break;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:				JAS_INFO("    ->Component type: FLOAT"); break;
			case TINYGLTF_COMPONENT_TYPE_INT:				JAS_INFO("    ->Component type: INT"); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:		JAS_INFO("    ->Component type: UNSIGNED_INT"); break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:				JAS_INFO("    ->Component type: SHORT"); break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:	JAS_INFO("    ->Component type: UNSIGNED_SHORT"); break;
			}
			JAS_INFO("    ->Component type size: {0} bytes", compSize);
			// type can be: TINYGLTF_TYPE_SCALAR, TINYGLTF_TYPE_VECTOR, TINYGLTF_TYPE_MATRIX, TINYGLTF_TYPE_VEC2, TINYGLTF_TYPE_VEC3, TINYGLTF_TYPE_VEC4, TINYGLTF_TYPE_MAT2, TINYGLTF_TYPE_MAT3, TINYGLTF_TYPE_MAT4
			JAS_INFO("    ->Type: 0x{0:b} ({1})", accessor.type, numComps);
			JAS_INFO("    ->Count: {0}", accessor.count);

			// Create buffer and bind it to memory.
			Buffer& buffer = mesh.attributes[attrib.first];
			buffer.init(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, queueIndices);
			mesh.bufferMemory.bindBuffer(&buffer);
		}
		mesh.bufferMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Upload data to GPU
		if (primitive.indices != -1)
		{
			tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
			size_t compSize = getComponentTypeSize(accessor.componentType);
			size_t numComps = getNumComponentsPerElement(accessor.type);
			size_t size = compSize * accessor.count * numComps;
			tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			tinygltf::Buffer& gltfBuffer = gltfModel.buffers[bufferView.buffer];
			unsigned char* indicesData = &gltfBuffer.data.at(0) + bufferView.byteOffset;
			mesh.bufferMemory.directTransfer(&mesh.indices, (void*)indicesData, (uint32_t)size, 0);
			vkDeviceWaitIdle(Instance::get().getDevice());
			//JAS_ERROR("INDEX count: {0}, compSize: {1}, #comps: {2}, size: {3}, offset: {4}", accessor.count, compSize, numComps, size, bufferView.byteOffset);
		}
		for (auto& attrib : primitive.attributes)
		{
			tinygltf::Accessor& accessor = gltfModel.accessors[attrib.second];
			size_t compSize = getComponentTypeSize(accessor.componentType);
			size_t numComps = getNumComponentsPerElement(accessor.type);
			size_t size = compSize * accessor.count * numComps;
			memSize += size;
			tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			tinygltf::Buffer& gltfBuffer = gltfModel.buffers[bufferView.buffer];
			unsigned char* bufferData = &gltfBuffer.data.at(0) + bufferView.byteOffset;

			Buffer& buffer = mesh.attributes[attrib.first];
			mesh.bufferMemory.directTransfer(&buffer, (void*)bufferData, (uint32_t)size, 0);
			vkDeviceWaitIdle(Instance::get().getDevice());
			//JAS_ERROR("{0} count: {1}, compSize: {2}, #comps: {3}, size: {4}, offset: {5}", attrib.first.c_str(), accessor.count, compSize, numComps, size, bufferView.byteOffset);

			// TODO: This can be different depending on mesh and can be more than just POSITION and TEXCOORD_0!!
			if (attrib.first == "POSITION")
			{
				static bool hasAddedPosAttribute = false;
				if (!hasAddedPosAttribute)
				{
					VertexAttribs::Attrib attrib;
					attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
					attrib.offset = 0;
					attrib.stride = sizeof(glm::vec3);
					attrib.location = 0;
					attrib.binding = 0;
					this->vertexAttribs.attribs.push_back(attrib);
					hasAddedPosAttribute = true;
				}
			}

			if (attrib.first == "TEXCOORD_0")
			{
				static bool hasAddedUVAttribute = false;
				if (!hasAddedUVAttribute)
				{
					VertexAttribs::Attrib attrib;
					attrib.format = VK_FORMAT_R32G32_SFLOAT;
					attrib.offset = 0;
					attrib.stride = sizeof(glm::vec2);
					attrib.location = 1;
					attrib.binding = 1;
					this->vertexAttribs.attribs.push_back(attrib);
					hasAddedUVAttribute = true;
				}
			}
		}
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
}

void GLTFLoader::loadNode(Model& model, Model::Node* node, tinygltf::Model& gltfModel, tinygltf::Node& gltfNode, std::string indents)
{
	//TODO: Load attribute buffers and index buffer into memory for use.
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

	// Check if it has a mesh
	if (gltfNode.mesh != -1)
	{
		// Set mesh
		node->mesh = &model.meshes[gltfNode.mesh];
	}

	if(!gltfNode.children.empty())
		JAS_INFO("{0} ->Children:", indents.c_str());
	node->children.resize(gltfNode.children.size());
	for (size_t childIndex = 0; childIndex < gltfNode.children.size(); childIndex++)
	{
		tinygltf::Node& child = gltfModel.nodes[gltfNode.children[childIndex]];
		loadNode(model, &node->children[childIndex], gltfModel, child, indents+"  ");
	}
}

glm::mat4 GLTFLoader::getViewFromCamera(float dt)
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

void GLTFLoader::drawNode(int index, Model::Node& node)
{
	if (node.mesh != nullptr)
	{
		VkBuffer vertexBuffers[] = { node.mesh->attributes["POSITION"].getBuffer(), node.mesh->attributes["TEXCOORD_0"].getBuffer() };
		VkDeviceSize vertOffsets[] = { 0, 0 };
		this->cmdBuffs[index]->cmdBindVertexBuffers(0, 2, vertexBuffers, vertOffsets);
		this->cmdBuffs[index]->cmdBindIndexBuffer(node.mesh->indices.getBuffer(), 0, VK_INDEX_TYPE_UINT16); // Unsinged short
		this->cmdBuffs[index]->cmdDrawIndexed(node.mesh->numIndices, 1, 0, 0, 0);
	}
	for (Model::Node& child : node.children)
		drawNode(index, child);
}

void GLTFLoader::loadSimpleModel()
{
	tinygltf::Model model;
	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";

	bool ret = false;
	size_t pos = filePath.rfind('.');
	if (pos != std::string::npos)
	{
		std::string err;
		std::string warn;

		std::string prefix = filePath.substr(pos);
		if (prefix == ".gltf")
			ret = this->loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
		else if (prefix == ".glb")
			ret = this->loader.LoadBinaryFromFile(&model, &err, &warn, filePath); // for binary glTF(.glb)

		if (!warn.empty()) JAS_WARN("GLTF Waring: {0}", warn.c_str());
		if (!err.empty()) JAS_ERROR("GLTF Error: {0}", err.c_str());
	}

	JAS_ASSERT(ret, "Failed to parse glTF\n");

	JAS_INFO("Loaded model");

	JAS_INFO("Processing data...");
	tinygltf::Scene & scene = model.scenes[0]; // nodes: 0
	JAS_INFO("Scene 0");

	tinygltf::Node & node = model.nodes[scene.nodes[0]]; // Name: Cube, Mesh: 0
	JAS_INFO("->Node: ", node.name.c_str());

	tinygltf::Mesh & mesh = model.meshes[node.mesh];
	JAS_INFO("	->Mesh: ", node.name.c_str());
	tinygltf::Primitive & primitive = mesh.primitives[0]; // Take first primitive. This holds the attributes, material, indices and the topology type (mode)
	JAS_ASSERT(primitive.mode == TINYGLTF_MODE_TRIANGLES, "Only support topology type TRIANGLE_LIST!");
	JAS_INFO("	  mode: TRIANGLES");

	// Indices (Assume it uses indices)
	tinygltf::Accessor & indicesAccessor = model.accessors[primitive.indices];
	tinygltf::BufferView & indicesView = model.bufferViews[indicesAccessor.bufferView];
	tinygltf::Buffer & indicesBuffer = model.buffers[indicesView.buffer];
	JAS_INFO("	  ->Fetched indices");
	unsigned char* indices = &indicesBuffer.data.at(0) + indicesView.byteOffset;

	// Positions
	tinygltf::Accessor & positionsAccessor = model.accessors[primitive.attributes["POSITION"]];
	tinygltf::BufferView & positionsView = model.bufferViews[positionsAccessor.bufferView];
	tinygltf::Buffer & positionsBuffer = model.buffers[positionsView.buffer];
	JAS_INFO("	  ->Fetched positions");
	unsigned char* positions = &positionsBuffer.data.at(0) + positionsView.byteOffset;

	// Texture coordinates
	tinygltf::Accessor & texCoordsAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
	tinygltf::BufferView & texCoordsView = model.bufferViews[texCoordsAccessor.bufferView];
	tinygltf::Buffer & texCoordsBuffer = model.buffers[texCoordsView.buffer];
	JAS_INFO("	  ->Fetched texCoords");
	unsigned char* texCoords = &texCoordsBuffer.data.at(0) + texCoordsView.byteOffset;

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
	};

	VertexAttribs::Attrib attrib;
	attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
	attrib.offset = offsetof(Vertex, pos);
	attrib.stride = sizeof(Vertex);
	attrib.location = 0;
	attrib.binding = 0;
	this->vertexAttribs.attribs.push_back(attrib);

	attrib.format = VK_FORMAT_R32G32_SFLOAT;
	attrib.offset = offsetof(Vertex, uv);
	attrib.stride = sizeof(Vertex);
	attrib.location = 1;
	attrib.binding = 0;
	this->vertexAttribs.attribs.push_back(attrib);

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

	this->memory.directTransfer(&this->bufferUniform, (void*)& uboData, unsiformBufferSize, 0);

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

size_t GLTFLoader::getComponentTypeSize(int type)
{
	size_t compSize = 0;
	switch (type)
	{
	case TINYGLTF_COMPONENT_TYPE_BYTE:				compSize = sizeof(int8_t);		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:		compSize = sizeof(uint8_t);		break;
	case TINYGLTF_COMPONENT_TYPE_DOUBLE:			compSize = sizeof(double);		break;
	case TINYGLTF_COMPONENT_TYPE_FLOAT:				compSize = sizeof(float);		break;
	case TINYGLTF_COMPONENT_TYPE_INT:				compSize = sizeof(int32_t);		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:		compSize = sizeof(uint32_t);	break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:				compSize = sizeof(int16_t);		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:	compSize = sizeof(uint16_t);	break;
	}
	return compSize;
}

size_t GLTFLoader::getNumComponentsPerElement(int type)
{
	size_t numComps = 0;
	switch (type)
	{
	case TINYGLTF_TYPE_SCALAR:	numComps = 1;	break;
	case TINYGLTF_TYPE_VECTOR:	numComps = 3;	break; // Unsure
	case TINYGLTF_TYPE_MATRIX:	numComps = 16;	break; // Unsure
	case TINYGLTF_TYPE_VEC2:	numComps = 2;	break;
	case TINYGLTF_TYPE_VEC3:	numComps = 3;	break;
	case TINYGLTF_TYPE_VEC4:	numComps = 4;	break;
	case TINYGLTF_TYPE_MAT2:	numComps = 4;	break;
	case TINYGLTF_TYPE_MAT3:	numComps = 9;	break;
	case TINYGLTF_TYPE_MAT4:	numComps = 16;	break;
	}
	return numComps;
}
