#pragma once

#include <vulkan/vulkan.h>

class ImageView
{
public:
	ImageView();
	~ImageView();

	void init(VkImage image, VkImageViewType type, VkFormat format);

	VkImageView getImageView() const { return this->imageView; }

	void cleanup();
private:
	VkImageView imageView;
};