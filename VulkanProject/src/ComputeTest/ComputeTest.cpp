#include "jaspch.h"
#include "ComputeTest.h"
#include "Vulkan/Instance.h"
#include "Core/Input.h"


#include <GLFW/glfw3.h>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

ComputeTest::ComputeTest()
{
}

ComputeTest::~ComputeTest()
{
}

void ComputeTest::init()
{
	Logger::init();

	this->window.init(1280, 720, "Vulkan Project");

	this->camera = new Camera(this->window.getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f);

	Instance::get().init(&this->window);
	this->swapChain.init(this->window.getWidth(), this->window.getHeight());

	initComputePipeline();
	JAS_INFO("Created Compute Pipeline!");
	initGraphicsPipeline();
	JAS_INFO("Created Graphics Pipeline!");

	this->commandPool.init(CommandPool::Queue::GRAPHICS, 0);

	this->framebuffers.resize(this->swapChain.getNumImages());

	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->swapChain.getImageViews()[i]);
		this->framebuffers[i].init(this->swapChain.getNumImages(), &this->renderPass, imageViews, this->swapChain.getExtent());
	}

	this->frame.init(&this->window,&this->swapChain);

	setupPostTEMP();
}

void ComputeTest::run()
{
	CommandBuffer* cmdBuffs[3];
	for (int i = 0; i < this->swapChain.getNumImages(); i++) {
		cmdBuffs[i] = this->commandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		cmdBuffs[i]->begin(0, nullptr);
		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);
		
		// Compute
		// Wait for vertex shader to finish using the memory the computer shader will manipulate
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = 0;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		std::vector<VkMemoryBarrier> memoryBarriers = { memoryBarrier };
		cmdBuffs[i]->cmdMemoryBarrier(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, memoryBarriers);

		cmdBuffs[i]->cmdBindPipeline(&this->computePipeline);
		std::vector<VkDescriptorSet> sets = { this->computeDescManager.getSet(i, 0) };
		std::vector<uint32_t> offsets;
		cmdBuffs[i]->cmdBindDescriptorSets(&this->computePipeline, 0, sets, offsets);

		const size_t NUM_PARTICLES_PER_WORKGROUP = 16;
		cmdBuffs[i]->cmdDispatch(std::ceilf((float)this->particles.size() / NUM_PARTICLES_PER_WORKGROUP), 1, 1);

		// Wait for compute shader to finish writing to the memory before vertex shader can read from it
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		memoryBarriers[0] = memoryBarrier;
		cmdBuffs[i]->cmdMemoryBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, memoryBarriers);
	
		// Graphics 
		cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, this->framebuffers[i].getFramebuffer(), this->swapChain.getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);
		cmdBuffs[i]->cmdBindPipeline(&this->pipeline);
		sets[0] = { this->descManager.getSet(i, 0) };
		cmdBuffs[i]->cmdBindDescriptorSets(&this->pipeline, 0, sets, offsets);
		const glm::vec4 tintData(0.3f, 0.8f, 0.4f, 1.0f);
		this->pushConstants[0].setDataPtr(&tintData[0]);
		cmdBuffs[i]->cmdPushConstants(&this->pipeline, &this->pushConstants[0]);
		cmdBuffs[i]->cmdDraw(100, 1, 0, 0);
		cmdBuffs[i]->cmdEndRenderPass();
		cmdBuffs[i]->end();
	}

	auto currentTime = std::chrono::high_resolution_clock::now();
	auto prevTime = currentTime;
	float dt = 0;
	unsigned frames = 0;
	double elapsedTime = 0;

	while (running && this->window.isOpen())
	{
		glfwPollEvents();

		this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);
		
		this->frame.beginFrame();

		ImGui::Begin("Hello world!");
		ImGui::Text("Cool text");
		ImGui::End();

		this->frame.submit(Instance::get().getGraphicsQueue().queue, cmdBuffs);
		this->frame.endFrame();
		this->camera->update(dt);
		
		currentTime = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration<float>(currentTime - prevTime).count();
		elapsedTime += dt;
		frames++;
		prevTime = currentTime;

		if (elapsedTime >= 1.0) {
			this->window.setTitle("FPS: " + std::to_string(frames) + " [Delta time: " + std::to_string((elapsedTime / frames) * 1000.f) + " ms]");
			elapsedTime = 0;
			frames = 0;
		}
	}
}

void ComputeTest::shutdown()
{
	vkDeviceWaitIdle(Instance::get().getDevice());

	// Compute
	this->particleBuffer.cleanup();
	this->particleMemory.cleanup();
	this->computeDescManager.cleanup();
	this->computeCommandPool.cleanup();
	this->computePipeline.cleanup();
	this->computeShader.cleanup();

	this->camBuffer.cleanup();
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

	delete this->camera;
}

void ComputeTest::setupPreTEMP()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.init();

	this->descManager.addLayout(this->descLayout);
	this->descManager.init(this->swapChain.getNumImages());

	PushConstants pushConsts;
	pushConsts.setLayout(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec4), 0);
	this->pushConstants.push_back(pushConsts);
}

void ComputeTest::setupPostTEMP()
{
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->camBuffer.init(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	// Create memory
	this->memory.bindBuffer(&this->camBuffer);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Update buffer
	this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);

	// Update descriptor
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		this->descManager.updateBufferDesc(0, 0, this->particleBuffer.getBuffer(), 0, sizeof(Particle) * this->particles.size());
		this->descManager.updateBufferDesc(0, 1, this->camBuffer.getBuffer(), 0, sizeof(glm::mat4));
		this->descManager.updateSets({ 0 }, i);
	}
}

void ComputeTest::generateParticleData()
{
	const unsigned NUM_PARTICLES = 100;
	this->particles.resize(NUM_PARTICLES);
	this->particleColors.resize(NUM_PARTICLES);

	std::uniform_real_distribution<float> uniform(-1.0f, 1.0f);
	std::mt19937 engine;
	for (unsigned i = 0; i < NUM_PARTICLES; i++)
	{
		this->particles[i].position = glm::vec3(0.2f * uniform(engine), 0.2f * uniform(engine), 0.2f * uniform(engine));
		float velocity = 0.00008f + 0.00003f * uniform(engine);
		float angle = 100.0f * uniform(engine);
		this->particles[i].velocity = (velocity * glm::vec3(glm::cos(angle), glm::sin(angle), glm::sin(angle)));

		float y = 0.8f + 0.2f * uniform(engine);
		float saturation = 0.8f + 0.2f * uniform(engine);
		float hue = 100.0f * uniform(engine);
		float u = saturation * glm::cos(hue);
		float v = saturation * glm::sin(hue);
		glm::vec3 rgb = glm::mat3(1.0f, 1.0f, 1.0f, 0.0f, -0.39465f, 2.03211f, 1.13983f, -0.58060f, 0.0f) * glm::vec3(y, u, v);
		this->particleColors[i] = glm::vec4(rgb, 0.4f);
	}

	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_COMPUTE_BIT, Instance::get().getPhysicalDevice()) };

	// Create buffer
	uint32_t size = sizeof(Particle) * this->particles.size();
	this->particleBuffer.init(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);

	// Create memory
	this->particleMemory.bindBuffer(&this->particleBuffer);
	this->particleMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Update buffer
	this->particleMemory.directTransfer(&this->particleBuffer, (void*)&this->particles[0], size, 0);

	// Update descriptor
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		this->computeDescManager.updateBufferDesc(0, 0, this->particleBuffer.getBuffer(), 0, size);
		this->computeDescManager.updateSets({0}, i);
	}
}

void ComputeTest::initComputePipeline()
{
	this->computeShader.addStage(Shader::Type::COMPUTE, "ComputeTest/computeTest.spv");
	this->computeShader.init();

	this->computeDescLayout.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr));
	this->computeDescLayout.init();

	this->computeDescManager.addLayout(this->computeDescLayout);
	this->computeDescManager.init(this->swapChain.getNumImages());
	generateParticleData();

	this->computePipeline.setDescriptorLayouts(this->computeDescManager.getLayouts());
	this->computePipeline.init(Pipeline::Type::COMPUTE, &this->computeShader);
}

void ComputeTest::initGraphicsPipeline()
{
	this->shader.addStage(Shader::Type::VERTEX, "ComputeTest/particleVertex.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, "ComputeTest/particleFragment.spv");
	this->shader.init();

	// Add attachments
	VkAttachmentDescription attachment = {};
	attachment.format = this->swapChain.getImageFormat();
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Change final layout to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL when using ImGui
	// as this renderpass will no longer be the last (and therefore not present) (VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	this->renderPass.addColorAttachment(attachment);

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
	this->pipeline.setPushConstants(this->pushConstants);
	this->pipeline.setDescriptorLayouts(this->descManager.getLayouts());
	PipelineInfo info = {};
	VkPipelineColorBlendAttachmentState blendAttachmentInfo = {};
	blendAttachmentInfo.blendEnable = true;
	blendAttachmentInfo.colorWriteMask = 0xf;
	blendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
	info.colorBlendAttachment = blendAttachmentInfo = {};

	VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.pNext = nullptr;
	colorBlendInfo.logicOpEnable = VK_TRUE;
	colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &blendAttachmentInfo;
	info.colorBlending = colorBlendInfo;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	info.inputAssembly = inputAssemblyInfo;

	this->pipeline.setPipelineInfo(PipelineInfoFlag::INPUT_ASSEMBLY, info);
	this->pipeline.setGraphicsPipelineInfo(this->swapChain.getExtent(), &this->renderPass);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
}
