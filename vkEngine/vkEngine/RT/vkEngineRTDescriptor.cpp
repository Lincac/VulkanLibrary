#include "vkEngineRTDescriptor.h"
#include "vkEngineRTHelp.h"

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

    VkDescriptorSetLayoutBinding bindings[6]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
        | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
        | VK_SHADER_STAGE_ANY_HIT_BIT_KHR
        | VK_SHADER_STAGE_MISS_BIT_KHR
        | VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 6;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkDescriptorPoolSize poolSizes[5]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = 2;
    poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[4].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 5;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void vkEngineRTDescriptor::setup(std::shared_ptr<vkEngineAccelerationStructure> tlas, std::shared_ptr<vkEngineImage> image,
    std::shared_ptr<vkEngineBuffer> vertexBuffer, std::shared_ptr<vkEngineTexture> environmentMap,
    std::shared_ptr<vkEngineBuffer> settingsBuffer)
{
    if (_layout == VK_NULL_HANDLE || _pool == VK_NULL_HANDLE) {
        throw std::runtime_error("descriptor layout/pool not created, call create() first");
    }
    if (!settingsBuffer) {
        throw std::runtime_error("path trace settings buffer is required (binding = 5)");
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

    VkDescriptorBufferInfo vertexBufferInfo{};
    vertexBufferInfo.buffer = vertexBuffer->getBuffer();
    vertexBufferInfo.offset = 0;
    vertexBufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet writeVertexBuffer{};
    writeVertexBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeVertexBuffer.dstSet = _set;
    writeVertexBuffer.dstBinding = 2;
    writeVertexBuffer.descriptorCount = 1;
    writeVertexBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeVertexBuffer.pBufferInfo = &vertexBufferInfo;

    VkDescriptorImageInfo envInfo{};
    envInfo.imageView = environmentMap->getImageView();
    envInfo.sampler = environmentMap->getSampler();
    envInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writeEnvironment{};
    writeEnvironment.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeEnvironment.dstSet = _set;
    writeEnvironment.dstBinding = 3;
    writeEnvironment.descriptorCount = 1;
    writeEnvironment.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeEnvironment.pImageInfo = &envInfo;

    VkDescriptorImageInfo envCdfInfo{};
    envCdfInfo.imageView = environmentMap->getEnvCdfImageView();
    envCdfInfo.sampler = environmentMap->getEnvCdfSampler();
    envCdfInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writeEnvCdf{};
    writeEnvCdf.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeEnvCdf.dstSet = _set;
    writeEnvCdf.dstBinding = 4;
    writeEnvCdf.descriptorCount = 1;
    writeEnvCdf.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeEnvCdf.pImageInfo = &envCdfInfo;

    VkDescriptorBufferInfo settingsBufferInfo{};
    settingsBufferInfo.buffer = settingsBuffer->getBuffer();
    settingsBufferInfo.offset = 0;
    settingsBufferInfo.range = sizeof(PathTraceSettingsGPU);

    VkWriteDescriptorSet writeSettings{};
    writeSettings.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSettings.dstSet = _set;
    writeSettings.dstBinding = 5;
    writeSettings.descriptorCount = 1;
    writeSettings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeSettings.pBufferInfo = &settingsBufferInfo;

    VkWriteDescriptorSet writes[6] = {
        writeAS, writeImage, writeVertexBuffer, writeEnvironment, writeEnvCdf, writeSettings
    };
    vkUpdateDescriptorSets(device, 6, writes, 0, nullptr);
}

VkDescriptorSetLayout& vkEngineRTDescriptor::getLayout()
{
    return _layout;
}

VkDescriptorSet& vkEngineRTDescriptor::getSet()
{
    return _set;
}
