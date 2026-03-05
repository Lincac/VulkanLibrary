#pragma once

#include <volk/volk.h>

class Image2D
{
public:

	Image2D(unsigned width, unsigned int height);

	void setImageFormat(VkFormat format);
	VkFormat getImageFormat() const;

	void setImageMipMapLevels(int level);
	int getImageMipMapLevels() const;

	void setImageArrayLayers(int layer);
	int getImageArrayLayers() const;

	void setImageUsageFlagBits(VkImageUsageFlagBits flags);
	VkImageUsageFlags getImageUsageFlagBits() const;

	void setImageMipMapLevelsVisit(int visit);
	int getImageMipMapLevelsVisit() const;

	void setImageArrayLayersVisit(int visit);
	int getImageArrayLayersVisit() const;

	void setImageAspectMask(VkImageAspectFlagBits bits);
	VkImageAspectFlags getImageAspectMask() const ;

private:

	VkImageCreateInfo _imgInfo;
	VkImageViewCreateInfo _imgViewInfo;

	VkImage _image;
	VkImageView _imageView;
};
