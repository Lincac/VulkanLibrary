#include "matVkEngineDescriptorSet.h"

#include <stdexcept>

namespace mat {

    VkDescriptorType toVkDescriptorType(DescriptorType type) {
        switch (type) {
            case DescriptorType::UBO:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::StorageBuffer:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case DescriptorType::Sampler2D:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            default:
                throw std::runtime_error("invalid descriptor type!");
        }
    }

    VkEngineDescriptorSet::VkEngineDescriptorSet() {}

    VkEngineDescriptorSet::~VkEngineDescriptorSet() {}

    void VkEngineDescriptorSet::create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                       std::shared_ptr<VkEngineDescriptorPool> pool,
                                       std::vector<std::shared_ptr<VkEngineDescriptorSetLayout>> layouts) {
        if (layouts.empty()) {
            throw std::runtime_error("descriptor set layouts is empty!");
        }

        std::vector<VkDescriptorSetLayout> vkLayouts;
        vkLayouts.reserve(layouts.size());
        for (const auto& layout : layouts) {
            vkLayouts.push_back(layout->getVkDescriptorSetLayout());
        }

        _descriptorSets.resize(layouts.size());

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool->getVkDescriptorPool();
        allocInfo.descriptorSetCount = static_cast<uint32_t>(vkLayouts.size());
        allocInfo.pSetLayouts = vkLayouts.data();

        if (vkAllocateDescriptorSets(logicalDevice->getVkDevice(), &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }

    void VkEngineDescriptorSet::updateBuffer(std::shared_ptr<VkEngineLogicalDevice> logicalDevice, uint32_t setIndex,
                                             uint32_t binding, DescriptorType type,
                                             std::shared_ptr<VkEngineBuffer> buffer, VkDeviceSize offset,
                                             VkDeviceSize range) {
        if (!buffer) {
            throw std::runtime_error("descriptor buffer is null!");
        }
        if (type != DescriptorType::UBO && type != DescriptorType::StorageBuffer) {
            throw std::runtime_error("invalid buffer descriptor type!");
        }

        DescriptorBufferWrite write{};
        write.setIndex = setIndex;
        write.binding = binding;
        write.type = type;
        write.buffer = buffer;
        write.offset = offset;
        write.range = range;

        updateBindings(logicalDevice, {write});
    }

    void VkEngineDescriptorSet::updateCombinedImageSampler(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                                           uint32_t setIndex, uint32_t binding,
                                                           std::shared_ptr<VkEngineImage> image, VkSampler sampler,
                                                           VkImageLayout layout) {
        if (!image) {
            throw std::runtime_error("descriptor image is null!");
        }
        if (sampler == VK_NULL_HANDLE) {
            throw std::runtime_error("descriptor sampler is null!");
        }

        DescriptorImageWrite write{};
        write.setIndex = setIndex;
        write.binding = binding;
        write.image = image;
        write.sampler = sampler;
        write.layout = layout;

        updateBindings(logicalDevice, {}, {write});
    }

    void VkEngineDescriptorSet::updateBindings(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                               const std::vector<DescriptorBufferWrite>& bufferWrites,
                                               const std::vector<DescriptorImageWrite>& imageWrites) {
        if (bufferWrites.empty() && imageWrites.empty()) {
            throw std::runtime_error("descriptor writes is empty!");
        }

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkWriteDescriptorSet> writes;
        bufferInfos.reserve(bufferWrites.size());
        imageInfos.reserve(imageWrites.size());
        writes.reserve(bufferWrites.size() + imageWrites.size());

        for (const auto& bufferWrite : bufferWrites) {
            validateSetIndex(bufferWrite.setIndex);
            if (!bufferWrite.buffer) {
                throw std::runtime_error("descriptor buffer is null!");
            }

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = bufferWrite.buffer->getVkBuffer();
            bufferInfo.offset = bufferWrite.offset;
            bufferInfo.range = bufferWrite.range;
            bufferInfos.push_back(bufferInfo);

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = _descriptorSets[bufferWrite.setIndex];
            write.dstBinding = bufferWrite.binding;
            write.dstArrayElement = 0;
            write.descriptorType = toVkDescriptorType(bufferWrite.type);
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferInfos.back();
            writes.push_back(write);
        }

        for (const auto& imageWrite : imageWrites) {
            validateSetIndex(imageWrite.setIndex);
            if (!imageWrite.image) {
                throw std::runtime_error("descriptor image is null!");
            }
            if (imageWrite.sampler == VK_NULL_HANDLE) {
                throw std::runtime_error("descriptor sampler is null!");
            }

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = imageWrite.sampler;
            imageInfo.imageView = imageWrite.image->getVkImageView();
            imageInfo.imageLayout = imageWrite.layout;
            imageInfos.push_back(imageInfo);

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = _descriptorSets[imageWrite.setIndex];
            write.dstBinding = imageWrite.binding;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imageInfos.back();
            writes.push_back(write);
        }

        vkUpdateDescriptorSets(logicalDevice->getVkDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0,
                               nullptr);
    }

    const std::vector<VkDescriptorSet>& VkEngineDescriptorSet::getVkDescriptorSets() const {
        return _descriptorSets;
    }

    void VkEngineDescriptorSet::validateSetIndex(uint32_t setIndex) const {
        if (setIndex >= _descriptorSets.size()) {
            throw std::runtime_error("descriptor set index out of range!");
        }
    }

}  // namespace mat
