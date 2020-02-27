#include "jaspch.h"
#include "ThreadingTest.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Vulkan/VulkanProfiler.h"
#include "Core/CPUProfiler.h"
#include "Core/Input.h"

#include "Models/GLTFLoader.h"

void ThreadingTest::init()
{
	VulkanProfiler::get().init(60, 60, VulkanProfiler::TimeUnit::MICRO);
	//VulkanProfiler::get().createTimestamps(14);
	//for(uint32_t t = 0; t < 7; t++)
	//	VulkanProfiler::get().addTimestamp("RecordThread_" + std::to_string(t));

	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->transferCommandPool.init(CommandPool::Queue::TRANSFER, 0);
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f);

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
	desc.pool = &this->graphicsCommandPool;
	this->depthTexture.getImage().transistionLayout(desc);

	this->pipeline.setPushConstants(this->pushConstants);
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

void ThreadingTest::loop(float dt)
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

void ThreadingTest::cleanup()
{
	this->threadManager.wait();
	VulkanProfiler::get().cleanup();
	this->stagingBuffer.cleanup();
	this->stagingMemory.cleanup();
	this->cameraBuffer.cleanup();
	this->memory.cleanup();
	this->transferCommandPool.cleanup();

	delete this->camera;
	for (ThreadData& tData : this->threadData)
	{
		delete tData.mutex;
		tData.cmdPool.cleanup();
	}
	this->graphicsCommandPool.cleanup();
	this->depthTexture.cleanup();
	this->imageMemory.cleanup();
	this->model.cleanup();
	this->descManager.cleanup();
	this->pipeline.cleanup();
	this->renderPass.cleanup();
	this->shader.cleanup();
}

void ThreadingTest::prepareBuffers()
{
	this->numThreads = this->threadManager.getMaxNumConcurrentThreads() - 1;
	this->threadManager.init(this->numThreads, getSwapChain()->getNumImages());

	// Utils function for random numbers.
	std::srand((unsigned int)std::time(NULL));
	auto rnd11 = [](int precision = 10000) { return (float)(std::rand() % precision) / (float)precision; };
	auto rnd = [rnd11](float min, float max) { return min + rnd11(RAND_MAX) * glm::abs(max - min); };
	static float RAD_PER_DEG = glm::pi<float>() / 180.f;

	static float RANGE = 15.f;
	static uint32_t NUM_OBJECTS = 10;

	// Create primary command buffers, one per swap chain image.
	this->primaryBuffers = this->graphicsCommandPool.createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	this->threadData.resize(this->numThreads);
	for (uint32_t t = 0; t < this->numThreads; t++)
	{
		// Create secondary buffers.
		ThreadData& tData = this->threadData[t];
		tData.cmdPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		tData.cmdBuffs.resize(getSwapChain()->getNumImages());
		for (uint32_t f = 0; f < getSwapChain()->getNumImages(); f++)
			tData.cmdBuffs[f] = tData.cmdPool.createCommandBuffers(2, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		tData.activeBuffers.resize(getSwapChain()->getNumImages(), 0);
		tData.mutex = new std::mutex();

		// Set the objects which the thread will render.
		glm::vec4 threadColor(rnd11(), rnd11(), rnd11(), 1.0f);
		uint32_t numObjsPerThread = (uint32_t)ceil((float)NUM_OBJECTS / (float)this->numThreads);
		for (uint32_t i = 0; i < numObjsPerThread; i++)
		{
			ObjectData obj;
			obj.model = glm::translate(glm::mat4(1.0f), { rnd(-RANGE, RANGE), rnd(-RANGE, RANGE), rnd(-RANGE, RANGE) });
			obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(1, 0.0f, 0.0f)));
			obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(0, 1.0f, 0.0f)));
			obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(0, 0.0f, 1.0f)));
			obj.model = glm::scale(obj.model, glm::vec3(rnd(0.1f, 1.0f), rnd(0.1f, 1.0f), rnd(0.1f, 1.0f)));
			obj.tint = threadColor;
			tData.objects.push_back(obj);
		}

		// Create push constants
		PushConstants pushConsts;
		pushConsts.setLayout(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstantData), 0);
		tData.pushConstants.push_back(pushConsts);
	}

	// Record initial secondary buffers
	for (size_t f = 0; f < this->getSwapChain()->getNumImages(); f++)
	{
		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = this->renderPass.getRenderPass();
		inheritanceInfo.framebuffer = getFramebuffers()[f].getFramebuffer();
		inheritanceInfo.subpass = 0;

		for (uint32_t t = 0; t < this->numThreads; t++)
		{
			// Add work to a specific thread, the thread has a queue which it will go through.
			this->threadManager.addWork(t, f, [=] { recordThread(t, f, inheritanceInfo); });
		}
	}

	this->threadManager.wait();
}

void ThreadingTest::recordThread(uint32_t threadId, uint32_t frameIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	JAS_PROFILER_SAMPLE_FUNCTION();
	ThreadData& tData = this->threadData[threadId];
	CommandBuffer* cmdBuff = nullptr;
	{
		std::lock_guard<std::mutex> lck(*tData.mutex);
		uint32_t activeBuffer = tData.activeBuffers[frameIndex] ^ 1;
		cmdBuff = tData.cmdBuffs[frameIndex][activeBuffer];
	}

	// Does not need to begin render pass because it inherits it form its primary command buffer.
	cmdBuff->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	//VulkanProfiler::get().startTimestamp("RecordThread_" + std::to_string(threadId), cmdBuff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	cmdBuff->cmdBindPipeline(&this->pipeline); // Think I need to do this, don't think this is inherited.

	const uint32_t numObjs = static_cast<uint32_t>(tData.objects.size());
	for (uint32_t i = 0; i < numObjs; i++)
	{
		JAS_PROFILER_SAMPLE_SCOPE("Obj_" + std::to_string(i));
		ObjectData& objData = tData.objects[i];

		// Apply push constants
		PushConstantData pData;
		pData.tint = objData.tint;
		pData.mw = objData.world * objData.model;
		tData.pushConstants[0].setDataPtr(&pData);
		cmdBuff->cmdPushConstants(&this->pipeline, &tData.pushConstants[0]);

		// Draw model
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(frameIndex, 0) };
		std::vector<uint32_t> offsets;
		GLTFLoader::recordDraw(&this->model, cmdBuff, &this->pipeline, sets, offsets); // Might need a model for each thread.
	}
	//VulkanProfiler::get().endTimestamp("RecordThread_" + std::to_string(threadId), cmdBuff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	cmdBuff->end();

	// Done recording tell main thread to use this buffer.
	{
		std::lock_guard<std::mutex> lck(*tData.mutex);
		tData.activeBuffers[frameIndex] ^= 1;
	}
}

void ThreadingTest::updateBuffers(uint32_t frameIndex, float dt)
{
	JAS_PROFILER_SAMPLE_FUNCTION();

	// Update camera
	glm::mat4 camMat = this->camera->getMatrix();
	this->memory.directTransfer(&this->cameraBuffer, (void*)&camMat, sizeof(glm::mat4), 0);

	// Record command buffers
	this->primaryBuffers[frameIndex]->begin(0, nullptr);
	//VulkanProfiler::get().resetAllTimestamps(this->primaryBuffers[frameIndex]);
	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);
	this->primaryBuffers[frameIndex]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[frameIndex].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	uint32_t nextFrame = (frameIndex + 1) % this->getSwapChain()->getNumImages();
	

	/*while (!this->threadManager.isDone(frameIndex)) {}

	glm::mat4 camMatrix = this->camera->getMatrix();
	for (uint32_t t = 0; t < this->numThreads; t++)
	{
		this->threadManager.addWork(t, nextFrame, [=] { recordThread(t, nextFrame, inheritanceInfo); });
	}*/
	static uint32_t objCount = 0;


	static uint32_t frameCount = 0;
	static bool keyWasPressed = false;
	if (Input::get().isKeyDown(GLFW_KEY_G))
		keyWasPressed = true;
	else if (keyWasPressed)
	{
		keyWasPressed = false;

		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = this->renderPass.getRenderPass();
		inheritanceInfo.subpass = 0;

		// Utils function for random numbers.
		std::srand((unsigned int)std::time(NULL));
		auto rnd11 = [](int precision = 10000) { return (float)(std::rand() % precision) / (float)precision; };
		auto rnd = [rnd11](float min, float max) { return min + rnd11(RAND_MAX) * glm::abs(max - min); };
		static float RAD_PER_DEG = glm::pi<float>() / 180.f;
		static float RANGE = 15.f;
		objCount = 0;
		for (uint32_t t = 0; t < this->numThreads; t++)
		{
			uint32_t numObjsPerThread = this->threadData[t].objects.size();
			glm::vec4 tint = this->threadData[t].objects.front().tint;
			for (uint32_t i = 0; i < numObjsPerThread; i++)
			{
				ObjectData obj;
				obj.model = glm::translate(glm::mat4(1.0f), { rnd(-RANGE, RANGE), rnd(-RANGE, RANGE), rnd(-RANGE, RANGE) });
				obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(1, 0.0f, 0.0f)));
				obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(0, 1.0f, 0.0f)));
				obj.model = glm::rotate(obj.model, RAD_PER_DEG * rnd(0.0f, 90.f), glm::normalize(glm::vec3(0, 0.0f, 1.0f)));
				obj.model = glm::scale(obj.model, glm::vec3(rnd(0.1f, 1.0f), rnd(0.1f, 1.0f), rnd(0.1f, 1.0f)));
				obj.tint = tint;
				this->threadData[t].objects.push_back(obj);
			}

			objCount += this->threadData[t].objects.size();

			inheritanceInfo.framebuffer = getFramebuffers()[0].getFramebuffer();
			this->threadManager.addWork(t, 0, [=] { recordThread(t, 0, inheritanceInfo); });

			inheritanceInfo.framebuffer = getFramebuffers()[1].getFramebuffer();
			this->threadManager.addWork(t, 1, [=] { recordThread(t, 1, inheritanceInfo); });

			inheritanceInfo.framebuffer = getFramebuffers()[2].getFramebuffer();
			this->threadManager.addWork(t, 2, [=] { recordThread(t, 2, inheritanceInfo); });
		}
	}

	ImGui::Begin("Number objects");
	ImGui::Text("Number of opbjects :%d", objCount);
	ImGui::End();


	std::vector<VkCommandBuffer> commandBuffers;
	for (ThreadData& tData : this->threadData)
	{
		std::lock_guard<std::mutex> lck(*tData.mutex);
		commandBuffers.push_back(tData.cmdBuffs[frameIndex][tData.activeBuffers[frameIndex]]->getCommandBuffer());
	}
	
	this->primaryBuffers[frameIndex]->cmdExecuteCommands(static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	this->primaryBuffers[frameIndex]->cmdEndRenderPass();
	this->primaryBuffers[frameIndex]->end();
}

void ThreadingTest::setupPre()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(getSwapChain()->getNumImages());

	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	GLTFLoader::loadToStagingBuffer(filePath, &this->model, &this->stagingBuffer, &this->stagingMemory);
	GLTFLoader::transferToModel(&this->transferCommandPool, &this->model, &this->stagingBuffer, &this->stagingMemory);

	PushConstants push;
	push.setLayout(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstantData), 0);
	this->pushConstants.push_back(push);
}

void ThreadingTest::setupPost()
{
	// Prepare camera buffer
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