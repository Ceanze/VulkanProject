#pragma once

#include <vulkan/vulkan.h>

class ImageView
{
public:
	ImageView(VkImage image, VkImageViewType type, VkFormat format);
	~ImageView();

	void cleanup();
private:
	VkImageView imageView;
};