#include "vkEngineRTDescriptor.h"

vkEngineRTDescriptor::vkEngineRTDescriptor(std::shared_ptr<vkEngineLogicalDevice> device)
    : _device(device)
{
}

vkEngineRTDescriptor::~vkEngineRTDescriptor()
{
    auto device = _device->getVkDevice();

    if (_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, _pool, nullptr);
        _pool = VK_NULL_HANDLE;
    }

    if (_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _layout, nullptr);
        _layout = VK_NULL_HANDLE;
    }
}

void vkEngineRTDescriptor::create()
{
    auto device = _device->getVkDevice();

    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void vkEngineRTDescriptor::setup(std::shared_ptr<vkEngineAccelerationStructure> tlas, std::shared_ptr<vkEngineImage> image)
{
    if (_layout == VK_NULL_HANDLE || _pool == VK_NULL_HANDLE) {
        throw std::runtime_error("descriptor layout/pool not created, call create() first");
    }

    auto device = _device->getVkDevice();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_layout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    VkAccelerationStructureKHR tlasHandle = tlas->getHandle();

    VkWriteDescriptorSetAccelerationStructureKHR asWrite{};
    asWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    asWrite.accelerationStructureCount = 1;
    asWrite.pAccelerationStructures = &tlasHandle;

    VkWriteDescriptorSet writeAS{};
    writeAS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeAS.dstSet = _set;
    writeAS.dstBinding = 0;
    writeAS.descriptorCount = 1;
    writeAS.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writeAS.pNext = &asWrite;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = image->getImageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writeImage{};
    writeImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeImage.dstSet = _set;
    writeImage.dstBinding = 1;
    writeImage.descriptorCount = 1;
    writeImage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeImage.pImageInfo = &imageInfo;

    VkWriteDescriptorSet writes[2] = { writeAS, writeImage };
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
}

VkDescriptorSetLayout& vkEngineRTDescriptor::getLayout()
{
    return _layout;
}

VkDescriptorSet& vkEngineRTDescriptor::getSet()
{
    return _set;
}
