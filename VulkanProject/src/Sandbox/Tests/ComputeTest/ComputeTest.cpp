#include "jaspch.h"
#include "ComputeTest.h"
#include "Vulkan/Instance.h"
#include "Core/Input.h"
#include "Vulkan/VulkanProfiler.h"


#include <GLFW/glfw3.h>
//#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define NUM_PARTICLES 10000

ComputeTest::ComputeTest()
{
}

ComputeTest::~ComputeTest()
{
}

void ComputeTest::init()
{
	this->camera = new Camera(getWindow()->getAspectRatio(), 45.f, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, 0.8f);
	
	setupDescriptors();

	getShaders().resize((unsigned)Shaders::SIZE);
	getPipelines().resize((unsigned)PipelineObject::SIZE);

	initComputePipeline();
	JAS_INFO("Created Compute Pipeline!");
	initGraphicsPipeline();
	JAS_INFO("Created Graphics Pipeline!");

	this->commandPool.init(CommandPool::Queue::GRAPHICS, 0);

	initFramebuffers(&this->renderPass, VK_NULL_HANDLE);

	VulkanProfiler::get().init(60, 60, VulkanProfiler::TimeUnit::MILLI);
	VulkanProfiler::get().createPipelineStats();
	VulkanProfiler::get().createTimestamps(4);
	VulkanProfiler::get().addTimestamp("Dispatch");
	VulkanProfiler::get().addTimestamp("Draw");

	for (int i = 0; i < getSwapChain()->getNumImages(); i++) {
		cmdBuffs[i] = this->commandPool.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		cmdBuffs[i]->begin(0, nullptr);

		// Profiler
		if (i == 0) { VulkanProfiler::get().resetAll(cmdBuffs[0]); }

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

		cmdBuffs[i]->cmdBindPipeline(&getPipeline((unsigned)PO::COMPUTE));
		std::vector<VkDescriptorSet> sets = { this->descManager.getSet(i, 1) };
		std::vector<uint32_t> offsets;
		cmdBuffs[i]->cmdBindDescriptorSets(&getPipeline((unsigned)PO::COMPUTE), 0, sets, offsets);

		const size_t NUM_PARTICLES_PER_WORKGROUP = 16;
		if (i == 0) { VulkanProfiler::get().startTimestamp("Dispatch", cmdBuffs[0], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT); }
		cmdBuffs[i]->cmdDispatch(std::ceilf((float)this->particles.size() / NUM_PARTICLES_PER_WORKGROUP), 1, 1);
		if (i == 0) { VulkanProfiler::get().endTimestamp("Dispatch", cmdBuffs[0], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT); }


		// Wait for compute shader to finish writing to the memory before vertex shader can read from it
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		memoryBarriers[0] = memoryBarrier;
		cmdBuffs[i]->cmdMemoryBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, memoryBarriers);

		// Graphics 
		if (i == 0) { VulkanProfiler::get().startPipelineStat(cmdBuffs[0]); }
		cmdBuffs[i]->cmdBeginRenderPass(&this->renderPass, getFramebuffers()[i].getFramebuffer(), getSwapChain()->getExtent(), clearValues, VK_SUBPASS_CONTENTS_INLINE);
		cmdBuffs[i]->cmdBindPipeline(&getPipeline((unsigned)PO::PARTICLE));
		sets[0] = { this->descManager.getSet(i, 0) };
		cmdBuffs[i]->cmdBindDescriptorSets(&getPipeline((unsigned)PO::PARTICLE), 0, sets, offsets);
		const glm::vec4 tintData(0.3f, 0.8f, 0.4f, 1.0f);
		this->pushConstants[0].setDataPtr(&tintData[0]);
		cmdBuffs[i]->cmdPushConstants(&getPipeline((unsigned)PO::PARTICLE), &this->pushConstants[0]);

		if (i == 0) { VulkanProfiler::get().startTimestamp("Draw", cmdBuffs[0], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT); }
		cmdBuffs[i]->cmdDraw(NUM_PARTICLES, 1, 0, 0);
		if (i == 0) { VulkanProfiler::get().endTimestamp("Draw", cmdBuffs[0], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT); }
		cmdBuffs[i]->cmdEndRenderPass();
		if (i == 0) { VulkanProfiler::get().endPipelineStat(cmdBuffs[0]); }
		cmdBuffs[i]->end();
	}
}

void ComputeTest::loop(float dt)
{
	this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);

	getFrame()->beginFrame(dt);
	getFrame()->submit(Instance::get().getGraphicsQueue().queue, cmdBuffs);
	getFrame()->endFrame();
	this->camera->update(dt);
}

void ComputeTest::cleanup()
{
	// Compute
	this->particleBuffer.cleanup();
	this->particleMemory.cleanup();
	this->camBuffer.cleanup();
	this->memory.cleanup();
	this->commandPool.cleanup();
	VulkanProfiler::get().cleanup();
	
	for (auto& pipeline : getPipelines())
		pipeline.cleanup();

	this->descManager.cleanup();
	this->renderPass.cleanup();

	for (auto& shader : getShaders())
		shader.cleanup();

	delete this->camera;
}

void ComputeTest::setupDescriptors()
{
	this->descLayout.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT, 1, nullptr));
	this->descLayout.init();
	// Set 0
	this->descManager.addLayout(this->descLayout);

	this->computeDescLayout.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT, 1, nullptr));
	this->computeDescLayout.init();
	// Set 1
	this->descManager.addLayout(this->computeDescLayout);

	this->descManager.init(getSwapChain()->getNumImages());

	generateParticleData();
	updateBufferDescs();

	PushConstants pushConsts;
	pushConsts.setLayout(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec4), 0);
	this->pushConstants.push_back(pushConsts);
}

void ComputeTest::updateBufferDescs()
{
	std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_GRAPHICS_BIT, Instance::get().getPhysicalDevice()) };
	this->camBuffer.init(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
	// Create memory
	this->memory.bindBuffer(&this->camBuffer);
	this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Update buffer
	this->memory.directTransfer(&this->camBuffer, (void*)&this->camera->getMatrix()[0], sizeof(glm::mat4), 0);

	// Update descriptor
	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++)
	{
		this->descManager.updateBufferDesc(0, 0, this->particleBuffer.getBuffer(), 0, sizeof(Particle) * this->particles.size());
		this->descManager.updateBufferDesc(0, 1, this->camBuffer.getBuffer(), 0, sizeof(glm::mat4));
		this->descManager.updateSets({ 0 }, i);
	}
}

void ComputeTest::generateParticleData()
{
	this->particles.resize(NUM_PARTICLES);

	std::uniform_real_distribution<float> uniform(-1.0f, 1.0f);
	std::mt19937 engine;
	for (unsigned i = 0; i < NUM_PARTICLES; i++)
	{
		this->particles[i].position = glm::vec3(0.2f * uniform(engine), 0.2f * uniform(engine), 0.2f * uniform(engine));
		float velocity = 0.0008f + 0.0003f * uniform(engine);
		float angle = 100.0f * uniform(engine);
		this->particles[i].velocity = (velocity * glm::vec3(glm::cos(angle), glm::sin(angle), glm::sin(angle)));
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
	for (size_t i = 0; i < getSwapChain()->getNumImages(); i++)
	{
		this->descManager.updateBufferDesc(1, 0, this->particleBuffer.getBuffer(), 0, size);
		this->descManager.updateSets({1}, i);
	}
}

void ComputeTest::initComputePipeline()
{

	getShader((unsigned)Shaders::COMPUTE).addStage(Shader::Type::COMPUTE, "ComputeTest/computeTest.spv");
	getShader((unsigned)Shaders::COMPUTE).init();

	std::vector<DescriptorLayout> setLayout = { this->descManager.getLayouts()[1] };
	
	getPipeline((unsigned)PO::COMPUTE).setDescriptorLayouts(setLayout);
	getPipeline((unsigned)PO::COMPUTE).init(Pipeline::Type::COMPUTE, &getShader((unsigned)Shaders::COMPUTE));
}

void ComputeTest::initGraphicsPipeline()
{
	getShader((unsigned)Shaders::PARTICLE).addStage(Shader::Type::VERTEX, "ComputeTest/particleVertex.spv");
	getShader((unsigned)Shaders::PARTICLE).addStage(Shader::Type::FRAGMENT, "ComputeTest/particleFragment.spv");
	getShader((unsigned)Shaders::PARTICLE).init();

	// Add attachments
	VkAttachmentDescription attachment = {};
	attachment.format = getSwapChain()->getImageFormat();
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

	
	getPipeline((unsigned)PO::PARTICLE).setPushConstants(this->pushConstants);

	std::vector<DescriptorLayout> setLayout = { this->descManager.getLayouts()[0] };
	getPipeline((unsigned)PO::PARTICLE).setDescriptorLayouts(setLayout);

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

	getPipeline((unsigned)PO::PARTICLE).setPipelineInfo(PipelineInfoFlag::INPUT_ASSEMBLY, info);
	getPipeline((unsigned)PO::PARTICLE).setGraphicsPipelineInfo(getSwapChain()->getExtent(), &this->renderPass);
	getPipeline((unsigned)PO::PARTICLE).init(Pipeline::Type::GRAPHICS, &getShader((unsigned)Shaders::PARTICLE));
}
