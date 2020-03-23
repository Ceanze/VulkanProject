#pragma once

#include "jaspch.h"
#include "Vulkan/Pipeline/DescriptorLayout.h"

class Texture;
class Sampler;
struct Material
{
	struct Tex
	{
		Texture* texture;
		Sampler* sampler;
	};

	Material();

	uint32_t index = -1;
	Tex baseColorTexture;
	Tex metallicRoughnessTexture;
	Tex normalTexture;
	Tex occlusionTexture;
	Tex emissiveTexture;

	/*
	struct PbrMR
	{
		struct BaseColorTexture
		{
			int index;
			int texCoord;
		} baseColorTexture;
		struct MetallicRoughnessTexture
		{
			int index;
			int texCoord;
		} metallicRoughnessTexture;
		glm::vec4 baseColorFactor;
		float metallicFactor;
		float roughnessFactor;
	} pbrMR;

	struct NormalTexture
	{
		int index;
		int texCoord;
		float scale;
	} normalTexture;

	// Where light occlude some of the material.
	struct OcclusionTexture
	{
		int index;
		int texCoord;
		float strength;
	} occlusionTexture;

	struct EmissiveTexture
	{
		int index;
		int texCoord;
	} emissiveTexture;
	glm::vec3 emissiveFactor;
	*/
	struct PushData
	{
		glm::vec4 baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
		glm::vec4 emissiveFactor = {1.0f, 1.0f, 1.0f, 1.0f};
		float metallicFactor = 0.0f;
		float roughnessFactor = 1.0f;
		// Does not support texCoord1 yet, only texCoord0 => assume every one uses texCoord0. This will be -1 if not set.
		int baseColorTextureCoord = -1;
		int metallicRoughnessTextureCoord = -1;
		int normalTextureCoord = -1;
		int occlusionTextureCoord = -1;
		int emissiveTextureCoord = -1;
		int _padding = 0;
	} pushData;

	//static DescriptorLayout descriptorLayout;
	static void initializeDescriptor(DescriptorLayout& descriptorLayout);
};