#include "Image2D.h"

Image2D::Image2D(unsigned width, unsigned int height)
{
	_imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	_imgInfo.imageType = VK_IMAGE_TYPE_2D;

	_imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	_imgInfo.extent = { width, height, 1 };
	_imgInfo.mipLevels = 1;
	_imgInfo.arrayLayers = 1;
	_imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

	_imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	_imgViewInfo.image = VK_NULL_HANDLE;
	_imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

	_imgViewInfo.format = _imgInfo.format;
	_imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	_imgViewInfo.subresourceRange.levelCount = 1;
	_imgViewInfo.subresourceRange.layerCount = 1;

	_image = VK_NULL_HANDLE;
	_imageView = VK_NULL_HANDLE;
}

void Image2D::setImageFormat(VkFormat format)
{
	_imgInfo.format = format;
	_imgViewInfo.format = format;
}

VkFormat Image2D::getImageFormat() const
{
	return _imgInfo.format;
}

void Image2D::setImageMipMapLevels(int level)
{
	_imgInfo.mipLevels = level;
}

int Image2D::getImageMipMapLevels() const
{
	return _imgInfo.mipLevels;
}

void Image2D::setImageArrayLayers(int layer)
{
	_imgInfo.arrayLayers = layer;
}

int Image2D::getImageArrayLayers() const
{
	return _imgInfo.arrayLayers;
}

void Image2D::setImageUsageFlagBits(VkImageUsageFlagBits bits)
{
	_imgInfo.usage = bits;
}

VkImageUsageFlags Image2D::getImageUsageFlagBits() const
{
	return _imgInfo.usage;
}

void Image2D::setImageMipMapLevelsVisit(int visit)
{
	_imgViewInfo.subresourceRange.levelCount = visit;
}

int Image2D::getImageMipMapLevelsVisit() const
{
	return _imgViewInfo.subresourceRange.levelCount;
}

void Image2D::setImageArrayLayersVisit(int visit)
{
	_imgViewInfo.subresourceRange.layerCount = visit;
}

int Image2D::getImageArrayLayersVisit() const
{
	return _imgViewInfo.subresourceRange.layerCount;
}

void Image2D::setImageAspectMask(VkImageAspectFlagBits bits)
{
	switch (_imgViewInfo.format)
	{
	case VK_FORMAT_R8G8B8A8_UNORM:
		if (bits == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			_imgViewInfo.subresourceRange.aspectMask = bits;
		}
		break;
	case VK_FORMAT_D32_SFLOAT:
		break;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		break;
	default:
		break;
	}
}

VkImageAspectFlags Image2D::getImageAspectMask() const
{
	return _imgViewInfo.subresourceRange.aspectMask;
}
