#include "jaspch.h"
#include "NoThreadingTest.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Vulkan/VulkanProfiler.h"
#include "Core/CPUProfiler.h"
#include "Core/Input.h"

#include "Models/GLTFLoader.h"
#include "Models/ModelRenderer.h"

void NoThreadingTest::init()
{
	VulkanProfiler::get().init(nullptr, 60, 60, VulkanProfiler::TimeUnit::MICRO);
	VulkanProfiler::get().createTimestamps(2);
	VulkanProfiler::get().addTimestamp("Main thread");

	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->transferCommandPool.init(CommandPool::Queue::TRANSFER, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f, 2.0f);

	this->shader.addStage(Shader::Type::VERTEX, "ThreadingTest\\threadingVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "ThreadingTest\\threadingFrag.spv");
	this->shader.init();

	// Add attachments
	this->renderPass.addDefaultColorAttachment(getSwapChain()->getImageFormat());
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

	setupPre();

	// Depth texture
	VkFormat depthFormat = findDepthFormat(Instance::get().getPhysicalDevice());
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->depthTexture.init(getSwapChain()->getExtent().width, getSwapChain()->getExtent().height,
		depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, queueIndices, 0, 1);

	// Bind image to memory.
	this->imageMemory.bindTexture(&this->depthTexture);
	this->imageMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Create image view for the depth texture.
	this->depthTexture.getImageView().init(this->depthTexture.getVkImage(), VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	// Transistion image
	Image::TransistionDesc desc;
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	desc.pool = &this->graphicsCommandPool;
	this->depthTexture.getImage().transistionLayout(desc);

	PushConstants& pushConstants = ModelRenderer::get().getPushConstants();
	this->pipeline.setPushConstants(pushConstants);
	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	this->pipeline.setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
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

	initFramebuffers(&this->renderPass, this->depthTexture.getVkImageView());

	setupPost();
}

void NoThreadingTest::loop(float dt)
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	static uint32_t frameCount = 0;
	static bool keyWasPressed = false;
	if (Input::get().isKeyDown(GLFW_KEY_I))
		keyWasPressed = true;
	else if (keyWasPressed)
	{
		keyWasPressed = false;
		frameCount = 0;
		Instrumentation::g_runProfilingSample = true;
	}
	if (Instrumentation::g_runProfilingSample)
	{
		frameCount++;
		if (frameCount > 10)
		{
			Instrumentation::g_runProfilingSample = false;
		}
	}

	getFrame()->beginFrame(dt);
	updateBuffers(getFrame()->getCurrentImageIndex(), dt);

	// Render
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->primaryBuffers.data());
	getFrame()->endFrame();
	this->camera->update(dt);
}

void NoThreadingTest::cleanup()
{
	VulkanProfiler::get().cleanup();
	this->stagingBuffers.cleanup();
	this->transferCommandPool.cleanup();
	this->cameraBuffer.cleanup();
	this->memory.cleanup();

	delete this->camera;
	this->graphicsCommandPool.cleanup();
	this->depthTexture.cleanup();
	this->imageMemory.cleanup();
	this->model.cleanup();
	this->descManager.cleanup();
	this->pipeline.cleanup();
	this->renderPass.cleanup();
	this->shader.cleanup();
}

void NoThreadingTest::prepareBuffers()
{
	// Utils function for random numbers.
	std::srand((unsigned int)std::time(NULL));
	auto rnd11 = [](int precision = 10000) { return (float)(std::rand() % precision) / (float)precision; };
	auto rnd = [rnd11](float min, float max) { return min + rnd11(RAND_MAX) * glm::abs(max - min); };
	static float RAD_PER_DEG = glm::pi<float>() / 180.f;

	static float RANGE = 15.f;
	static uint32_t NUM_OBJECTS = 1000;

	// Create primary command buffers, one per swap chain image.
	this->primaryBuffers = this->graphicsCommandPool.createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// Set the objects which the thread will render.
	glm::vec4 threadColor(rnd11(), rnd11(), rnd11(), 1.0f);
	for (uint32_t i = 0; i < NUM_OBJECTS; i++)
	{
		ObjectData obj;
		obj.model = glm::translate(glm::mat4(1.0f), { rnd(-RANGE, RANGE), rnd(-RANGE, RANGE), rnd(-RANGE, RANGE) });
		obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(1, 0.0f, 0.0f)));
		obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(0, 1.0f, 0.0f)));
		obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(0, 0.0f, 1.0f)));
		obj.model = glm::scale(obj.model, glm::vec3(rnd(0.1f, 1.0f), rnd(0.1f, 1.0f), rnd(0.1f, 1.0f)));
		obj.tint = threadColor;
		this->objects.push_back(obj);
	}
}

void NoThreadingTest::recordThread(uint32_t frameIndex)
{
	JAS_PROFILER_SAMPLE_FUNCTION();

	// Does not need to begin render pass because it inherits it form its primary command buffer.
	VulkanProfiler::get().startTimestamp("Main thread", this->primaryBuffers[frameIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	this->primaryBuffers[frameIndex]->cmdBindPipeline(&this->pipeline); // Think I need to do this, don't think this is inherited.

	for (uint32_t i = 0; i < 1000; i++)
	{
		JAS_PROFILER_SAMPLE_SCOPE("Obj_" + std::to_string(i));
		ObjectData & objData = this->objects[i];

		// Apply push constants
		PushConstantData pData;
		pData.tint = objData.tint;
		pData.mw = objData.world * objData.model;

		// Draw model
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(frameIndex, 0) };
		std::vector<uint32_t> offsets;
		uint32_t pushOffset = ModelRenderer::get().getPushConstantSize();
		this->primaryBuffers[frameIndex]->cmdPushConstants(&this->pipeline, VK_SHADER_STAGE_VERTEX_BIT, pushOffset, sizeof(PushConstantData), &pData);
		ModelRenderer::get().record(&this->model, glm::mat4(1.0f), this->primaryBuffers[frameIndex], &this->pipeline, sets, offsets);
		//GLTFLoader::recordDraw(&this->model, this->primaryBuffers[frameIndex], &this->pipeline, sets, offsets); // Might need a model for each thread.
	}
	VulkanProfiler::get().endTimestamp("Main thread", this->primaryBuffers[frameIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

void NoThreadingTest::updateBuffers(uint32_t frameIndex, float dt)
{
	JAS_PROFILER_SAMPLE_FUNCTION();

	// Update camera
	glm::mat4 camMat = this->camera->getMatrix();
	this->memory.directTransfer(&this->cameraBuffer, (void*)& camMat, sizeof(glm::mat4), 0);

	// Record command buffers
	this->primaryBuffers[frameIndex]->begin(0, nullptr);
	VulkanProfiler::get().resetAllTimestamps(this->primaryBuffers[frameIndex]);
	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);
	this->primaryBuffers[frameIndex]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[frameIndex].getFramebuffer(), 
		getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);

	recordThread(frameIndex);

	this->primaryBuffers[frameIndex]->cmdEndRenderPass();
	this->primaryBuffers[frameIndex]->end();
}

void NoThreadingTest::setupPre()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Camera
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(getSwapChain()->getNumImages());

	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	
	GLTFLoader::prepareStagingBuffer(filePath, &this->model, &this->stagingBuffers);
	this->stagingBuffers.initMemory();
	GLTFLoader::transferToModel(&this->transferCommandPool, &this->model, &this->stagingBuffers);

	uint32_t pushOffset = ModelRenderer::get().getPushConstantSize();
	ModelRenderer::get().addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstantData), pushOffset);
	ModelRenderer::get().init();
}

void NoThreadingTest::setupPost()
{
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->cameraBuffer.init(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);

	this->memory.bindBuffer(&this->cameraBuffer);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Update descriptor
	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++)
	{
		VkDeviceSize vertexBufferSize = this->model.vertices.size() * sizeof(Vertex);
		this->descManager.updateBufferDesc(0, 0, this->model.vertexBuffer.getBuffer(), 0, vertexBufferSize);
		this->descManager.updateBufferDesc(0, 1, this->cameraBuffer.getBuffer(), 0, sizeof(glm::mat4));
		this->descManager.updateSets({ 0 }, i);
	}

	prepareBuffers();
}