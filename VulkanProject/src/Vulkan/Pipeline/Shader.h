#pragma once

#include "jaspch.h"

#define JAS_PATH_TO_SHADERS "..\\assets\\Shaders\\"

class Shader
{
public:
	enum Type { VERTEX, FRAGMENT, COMPUTE };

	Shader();
	virtual ~Shader();

	void setName(const std::string& name);
	void addStage(Type type, const std::string& shaderName);
	void init();

	void cleanup();

	VkPipelineShaderStageCreateInfo getShaderCreateInfo(Type type) const;
	std::string getName() const;
	uint32_t getId() const;

private:
	struct Stage
	{
		VkPipelineShaderStageCreateInfo stageCreateInfo;
		VkShaderModule shaderModule;
	};

	VkShaderModule createShaderModule(Type type, const std::vector<char>& bytecode);
	VkPipelineShaderStageCreateInfo createShaderStageCreateInfo(Type type, VkShaderModule shaderModule);

	static VkShaderStageFlagBits typeToStageFlag(Type type);
	static std::string typeToStr(Type type);
	static std::vector<char> readFile(const std::string& filename);

private:
	std::unordered_map<Type, Stage> shaderStages;
	std::unordered_map<Type, std::string> shaderStageFilePaths;

	std::string name;
	uint32_t id;
};