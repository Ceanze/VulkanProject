#include "jaspch.h"
#include "Shader.h"

#include "../Instance.h"

#include <fstream>

Shader::Shader()
{
	// Set its id
	static uint32_t id = 0;
	this->id = id++;

	// Set default name
	this->name = "Shader#" + std::to_string(this->id);
}

Shader::~Shader()
{
}

void Shader::setName(const std::string& name)
{
	this->name = name;
}

void Shader::addStage(Type type, const std::string& shaderName)
{
	auto& it = this->shaderStages.find(type);
#ifdef JAS_DEBUG
	if (it != this->shaderStages.end())
		JAS_WARN("Overwriting shader stage {0} in shader {1}!", typeToStr(type).c_str(), this->name.c_str());
#endif

	std::string filePath = JAS_PATH_TO_SHADERS + shaderName;
	this->shaderStageFilePaths[type] = filePath;
}

void Shader::init()
{
	// Create all shader modules and its create info.
	for (auto stageInfo : this->shaderStageFilePaths)
	{
		const Type type = stageInfo.first;
		Stage stage = {};

		std::vector<char> bytecode = readFile(stageInfo.second);
		stage.shaderModule = createShaderModule(type, bytecode);
		stage.stageCreateInfo = createShaderStageCreateInfo(type, stage.shaderModule);

		this->shaderStages[type] = stage;
	}
}

void Shader::cleanup()
{
	// Destroy all shader modules.
	for (auto& pair : this->shaderStages)
		vkDestroyShaderModule(Instance::get().getDevice(), pair.second.shaderModule, nullptr);

	this->shaderStages.clear();
}

VkPipelineShaderStageCreateInfo Shader::getShaderCreateInfo(Type type) const
{
	auto it = this->shaderStages.find(type);
#ifdef JAS_DEBUG
	if (it == this->shaderStages.end())
		JAS_ASSERT(false, "Type {0} passed to getShaderCreateInfo is not supported in shader {1}!", typeToStr(type).c_str(), this->name.c_str());
#endif
	return it->second.stageCreateInfo;
}

std::vector<VkPipelineShaderStageCreateInfo> Shader::getShaderCreateInfos()
{
	std::vector<VkPipelineShaderStageCreateInfo> infos;
	for (auto stage : this->shaderStages)
		infos.push_back(stage.second.stageCreateInfo);
	
	return infos;
}

std::string Shader::getName() const
{
	return std::string();
}

uint32_t Shader::getId() const
{
	return this->id;
}

VkShaderModule Shader::createShaderModule(Type type, const std::vector<char>& bytecode)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = bytecode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(Instance::get().getDevice(), &createInfo, nullptr, &shaderModule);
	JAS_ASSERT(result == VK_SUCCESS, "Failed to create shader module of shader stage {0} to shader {1}!", typeToStr(type).c_str(), this->name.c_str());

	return shaderModule;
}

VkPipelineShaderStageCreateInfo Shader::createShaderStageCreateInfo(Type type, VkShaderModule shaderModule)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = typeToStageFlag(type);
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr; // May be used to create different constant for techniques

	return vertShaderStageInfo;
}

VkShaderStageFlagBits Shader::typeToStageFlag(Type type)
{
	switch (type)
	{
	case Type::VERTEX: return VK_SHADER_STAGE_VERTEX_BIT; break;
	case Type::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT; break;
	case Type::COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT; break;
	default:
		JAS_ERROR("Shader type not supported!");
		return VK_SHADER_STAGE_VERTEX_BIT;
		break;
	}
}

std::string Shader::typeToStr(Type type)
{
	switch (type)
	{
	case Type::VERTEX: return "VERTEX"; break;
	case Type::FRAGMENT: return "FRAGMENT"; break;
	case Type::COMPUTE: return "COMPUTE"; break;
	default:
		JAS_ERROR("Shader type not supported!");
		return "UNKNOWN TYPE";
		break;
	}
}

std::vector<char> Shader::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	JAS_ASSERT(file.is_open(), "Failed to open file {0}!", filename.c_str());

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
