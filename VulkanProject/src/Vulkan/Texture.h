#pragma once

#include "jaspch.h"
#include "Buffers/Image.h"
#include "Buffers/ImageView.h"

class Texture
{
public:
	Texture();
	~Texture();

	void init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices, VkImageCreateFlags flags, uint32_t arrayLayers);
	void cleanup();

	VkMemoryRequirements getMemReq() const;
	VkImage getVkImage() const;
	Image& getImage();
	ImageView& getImageView();
	VkImageView getVkImageView() const;

	VkFormat getFormat() const;
	uint32_t getWidth() const;
	uint32_t getHeight() const;

private:
	Image image;
	ImageView imageView;
	uint32_t width, height;
	VkFormat format;
};