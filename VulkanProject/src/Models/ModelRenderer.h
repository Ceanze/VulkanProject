
#include "Models/Model/Model.h"
#include "Vulkan/Pipeline/PushConstants.h"
#include "Vulkan/Pipeline/Pipeline.h"

class CommandBuffer;
class Pipeline;

class ModelRenderer
{
public:
	static ModelRenderer& get();
	~ModelRenderer();
	
	/*
		ModelRender has its own push constants at the start of the data. You need to offset your push constants with getPushConstantSize()!
	*/
	void record(Model* model, glm::mat4 transform, CommandBuffer* commandBuffer, Pipeline* pipeline, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets);

	void init();

	void addPushConstant(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset);

	PushConstants& getPushConstants();
	uint32_t getPushConstantSize() const;

private:
	struct PushConstantData
	{
		glm::mat4 matrix;
	};

	ModelRenderer();
	void drawNode(CommandBuffer* commandBuffer, Pipeline* pipeline, Model::Node& node, glm::mat4 transform);
	
private:
	PushConstants pushConstants;
	uint32_t size;
};