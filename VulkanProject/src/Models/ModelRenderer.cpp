#include "jaspch.h"
#include "ModelRenderer.h"
#include "Vulkan/CommandBuffer.h"

ModelRenderer& ModelRenderer::get()
{
	static ModelRenderer modelRenderer;
	return modelRenderer;
}

ModelRenderer::~ModelRenderer()
{
	this->pushConstants.cleanup();
}

void ModelRenderer::record(Model* model, glm::mat4 transform, CommandBuffer* commandBuffer, Pipeline* pipeline, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets, uint32_t instanceCount)
{
	// TODO: Use different materials, can still use same pipeline if all meshes uses same type of material (i.e. PBR)!
	if (model->vertexBuffer.getBuffer() == VK_NULL_HANDLE)
		return;

	//VkBuffer vertexBuffers[] = { this->model.vertexBuffer.getBuffer() };
	//VkDeviceSize vertOffsets[] = { 0 };
	//this->cmdBuffs[i]->cmdBindVertexBuffers(0, 1, vertexBuffers, vertOffsets);
	if (model->indices.empty() == false)
		commandBuffer->cmdBindIndexBuffer(model->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	commandBuffer->cmdBindDescriptorSets(pipeline, 0, sets, offsets);
	for (Model::Node& node : model->nodes)
		drawNode(commandBuffer, pipeline, node, transform, instanceCount);
}

void ModelRenderer::init()
{
	this->pushConstants.init();
}

void ModelRenderer::addPushConstant(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
{
	this->pushConstants.addLayout(stageFlags, size, offset);
}

PushConstants& ModelRenderer::getPushConstants()
{
	return this->pushConstants;
}

uint32_t ModelRenderer::getPushConstantSize() const
{
	return this->size;
}

ModelRenderer::ModelRenderer() : size(0)
{
	// Vertex push constants
	this->pushConstants.addLayout(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstantData), 0);
	this->size = this->pushConstants.getSize();
}

void ModelRenderer::drawNode(CommandBuffer* commandBuffer, Pipeline* pipeline, Model::Node& node, glm::mat4 transform, uint32_t instanceCount)
{
	if (node.hasMesh)
	{
		// Set transformation matrix
		PushConstantData pushConstantData;
		pushConstantData.matrix = node.parent != nullptr ? (transform * node.parent->matrix * node.matrix) : (transform * node.matrix);
		commandBuffer->cmdPushConstants(pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pushConstantData);

		Mesh& mesh = node.mesh;
		for (Primitive& primitive : mesh.primitives)
		{
			// TODO: Send node transformation as push constant per draw!

			if (primitive.hasIndices)
				commandBuffer->cmdDrawIndexed(primitive.indexCount, instanceCount, primitive.firstIndex, 0, 0);
			else
				commandBuffer->cmdDraw(primitive.vertexCount, instanceCount, 0, 0);
		}
	}

	// Draw child nodes
	for (Model::Node& child : node.children)
		drawNode(commandBuffer, pipeline, child, transform, instanceCount);
}
