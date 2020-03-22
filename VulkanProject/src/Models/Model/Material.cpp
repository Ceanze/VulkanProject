#include "jaspch.h"
#include "Material.h"

Material::Material()
{
	
}

void Material::initializeDescriptor(DescriptorLayout& descriptorLayout)
{
	// Textures
	descriptorLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr)); // BaseColor
	descriptorLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr)); // MetallicRoughness
	descriptorLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr)); // NormalTexture
	descriptorLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr)); // OcclusionTexture
	descriptorLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT, 1, nullptr)); // EmissiveTexture
	descriptorLayout.init();
}