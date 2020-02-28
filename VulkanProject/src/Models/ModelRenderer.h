
#include "Models/Model/Model.h"
#include "Vulkan/Pipeline/PushConstants.h"
#include "Vulkan/Pipeline/Pipeline.h"

class CommandBuffer;
class Pipeline;

class ModelRenderer
{
public:
	static ModelRenderer& get();
	
	void record(Model* model, glm::mat4 transform, CommandBuffer* commandBuffer, Pipeline* pipeline, const std::vector<VkDescriptorSet>& sets, const std::vector<uint32_t>& offsets);

	void init();

	void addPushconstant(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset);
	void setPushconstantData(void* data, uint32_t size, uint32_t offset);

	std::vector<PushConstants> getPushConstants() const;

private:
	struct PushConstantData
	{
		glm::mat4 matrix;
	};

	ModelRenderer();
	void drawNode(CommandBuffer* commandBuffer, Pipeline* pipeline, Model::Node& node, glm::mat4 transform);
	
private:
	
	PushConstants pushConstants;
};