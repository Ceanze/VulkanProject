#include "jaspch.h"
#include "ThreadingTest.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Models/GLTFLoader.h"

void ThreadingTest::init()
{
	this->graphicsCommandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
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
	getFrame()->beginFrame(dt);
	updateBuffers(getFrame()->getCurrentImageIndex(), dt);

	// Render
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, this->primaryBuffers.data());
	getFrame()->endFrame();
	this->camera->update(dt);
}

void ThreadingTest::cleanup()
{
	delete this->camera;
	for (ThreadData& tData : this->threadData)
		tData.cmdPool.cleanup();
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
	this->numThreads = this->threadManager.getMaxNumConcurrentThreads();
	this->threadManager.init(this->numThreads);

	// Utils function for random numbers.
	std::srand((unsigned int)std::time(NULL));
	auto rnd11 = [](int precision = 10000) { return (float)(std::rand() % precision) / (float)precision; };
	auto rnd = [rnd11](float min, float max) { return min + rnd11(RAND_MAX) * glm::abs(max - min); };
	static float RAD_PER_DEG = glm::pi<float>() / 180.f;

	static float RANGE = 15.f;
	static uint32_t NUM_OBJECTS = 500;

	// Create primary command buffers, one per swap chain image.
	this->primaryBuffers = this->graphicsCommandPool.createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	this->threadData.resize(this->numThreads);
	for (uint32_t t = 0; t < this->numThreads; t++)
	{
		// Create secondary buffers.
		ThreadData& tData = this->threadData[t];
		tData.cmdPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		tData.cmdBuffs = tData.cmdPool.createCommandBuffers(getSwapChain()->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

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
}

void ThreadingTest::recordThread(uint32_t threadId, uint32_t frameIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	ThreadData& tData = this->threadData[threadId];
	CommandBuffer* cmdBuff = tData.cmdBuffs[frameIndex];

	// Does not need to begin render pass because it inherits it form its primary command buffer.
	cmdBuff->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	cmdBuff->cmdBindPipeline(&this->pipeline); // Think I need to do this, don't think this is inherited.

	const uint32_t numObjs = static_cast<uint32_t>(tData.objects.size());
	for (uint32_t i = 0; i < numObjs; i++)
	{
		ObjectData& objData = tData.objects[i];

		// Apply push constants
		PushConstantData pData;
		pData.vp = this->camera->getMatrix();
		pData.tint = objData.tint;
		pData.mw = objData.world * objData.model;
		tData.pushConstants[0].setDataPtr(&pData);
		cmdBuff->cmdPushConstants(&this->pipeline, &tData.pushConstants[0]);

		// Draw model
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(frameIndex, 0) };
		std::vector<uint32_t> offsets;
		GLTFLoader::recordDraw(&this->model, cmdBuff, &this->pipeline, sets, offsets); // Might need a model for each thread.
	}

	cmdBuff->end();
}

void ThreadingTest::updateBuffers(uint32_t frameIndex, float dt)
{
	// Record command buffers
	this->primaryBuffers[frameIndex]->begin(0, nullptr);
	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);
	this->primaryBuffers[frameIndex]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[frameIndex].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = this->renderPass.getRenderPass();
	inheritanceInfo.framebuffer = getFramebuffers()[frameIndex].getFramebuffer();
	inheritanceInfo.subpass = 0;

	/*
	// ----------------- INIT -----------------
	// Set number of threads and the threads data structure
	struct DataStructure
	{
		CommandPool cmdPool;
		std::vector<CommandBuffer*> cmdBuffs;
		std::vector<ObjectData> objects;
		std::vector<PushConstants> pushConstants;
		WorkType workType;
	} dataStruct[numThreads];
	this->threadManager.setNumThreads(numThreads, dataStruct);
	commandBuffers.resize(numThreads);
	// ----------------- LOOP -----------------
	// Get threads which are done with their work.
	auto& freeThreads = this->threadManager.getFreeThread();
	for(auto& freeThread : freeThreads)
		if(worklist)
		commandBuffers[freeThread.workIndex] = freeThread.cmdBuffs[frameIndex]->getCommandBuffer();
	submit();

	// Fetch their work.
	std::vector<VkCommandBuffer> commandBuffers;
	std::sort(freeThreads, [](auto a, auto b) { return a.workType < b.workType;})
	for(auto& freeThread : freeThreads)
		commandBuffers.push_back(freeThread.cmdBuffs[frameIndex]->getCommandBuffer());
	submit()
	// Add new work for them.
	for(auto& freeThread : freeThreads)
	{
		workList
	}
	freeThread.addWork([=] { recordThread(t, frameIndex, inheritanceInfo); });

	Time A 
		T -> A
		C -> A -> R

	Time B
		T -> B
		C -> B -> R

	Render 

	WorkType:
		TRANSFER ( <- Mesh)
		COMPUTE (Culling of Mesh)
		RENDER (IndirectDraw on Mesh buffer)

	workList ordning viktig!
	commandBuffers.resize(workList.size())
	*/

	for (uint32_t t = 0; t < this->numThreads; t++)
	{
		// Add work to a specific thread, the thread has a queue which it will go through.
		this->threadManager.addWork(t, [=] { recordThread(t, frameIndex, inheritanceInfo); });
	}

	this->threadManager.wait();

	std::vector<VkCommandBuffer> commandBuffers;
	for (ThreadData& tData : this->threadData)
		commandBuffers.push_back(tData.cmdBuffs[frameIndex]->getCommandBuffer());

	this->primaryBuffers[frameIndex]->cmdExecuteCommands(static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	this->primaryBuffers[frameIndex]->cmdEndRenderPass();
	this->primaryBuffers[frameIndex]->end();
}

void ThreadingTest::setupPre()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr)); // Vertices
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(getSwapChain()->getNumImages());

	const std::string filePath = "..\\assets\\Models\\Cube\\Cube.gltf";
	GLTFLoader::load(filePath, &this->model);

	PushConstants pushConsts;
	pushConsts.setLayout(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstantData), 0);
	this->pushConstants.push_back(pushConsts);
}

void ThreadingTest::setupPost()
{
	// Update descriptor
	for (uint32_t i = 0; i < getSwapChain()->getNumImages(); i++)
	{
		VkDeviceSize vertexBufferSize = this->model.vertices.size() * sizeof(Vertex);
		this->descManager.updateBufferDesc(0, 0, this->model.vertexBuffer.getBuffer(), 0, vertexBufferSize);
		this->descManager.updateSets({ 0 }, i);
	}

	prepareBuffers();
}