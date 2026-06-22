#pragma once

#include "resource/matVkEngineBuffer.h"
#include "resource/matVkEngineImage.h"
#include "shader/matVkEngineDescriptorPool.h"

namespace mat {

    struct DescriptorBufferWrite {
        uint32_t setIndex = 0;
        uint32_t binding = 0;
        DescriptorType type = DescriptorType::None;
        std::shared_ptr<VkEngineBuffer> buffer;
        VkDeviceSize offset = 0;
        VkDeviceSize range = VK_WHOLE_SIZE;
    };

    struct DescriptorImageWrite {
        uint32_t setIndex = 0;
        uint32_t binding = 0;
        std::shared_ptr<VkEngineImage> image;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    };

    class VkEngineDescriptorSet {
    public:
        VkEngineDescriptorSet();
        ~VkEngineDescriptorSet();

        void create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice, std::shared_ptr<VkEngineDescriptorPool> pool,
                    std::vector<std::shared_ptr<VkEngineDescriptorSetLayout>> layouts);

        void updateBuffer(std::shared_ptr<VkEngineLogicalDevice> logicalDevice, uint32_t setIndex, uint32_t binding,
                          DescriptorType type, std::shared_ptr<VkEngineBuffer> buffer, VkDeviceSize offset = 0,
                          VkDeviceSize range = VK_WHOLE_SIZE);

        void updateCombinedImageSampler(std::shared_ptr<VkEngineLogicalDevice> logicalDevice, uint32_t setIndex,
                                        uint32_t binding, std::shared_ptr<VkEngineImage> image, VkSampler sampler,
                                        VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        void updateBindings(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                            const std::vector<DescriptorBufferWrite>& bufferWrites,
                            const std::vector<DescriptorImageWrite>& imageWrites = {});

        const std::vector<VkDescriptorSet>& getVkDescriptorSets() const;

    private:
        VkEngineDescriptorSet(const VkEngineDescriptorSet&) = delete;
        VkEngineDescriptorSet(VkEngineDescriptorSet&&) = delete;
        VkEngineDescriptorSet& operator=(const VkEngineDescriptorSet&) = delete;
        VkEngineDescriptorSet& operator=(VkEngineDescriptorSet&&) = delete;

        void validateSetIndex(uint32_t setIndex) const;

        std::vector<VkDescriptorSet> _descriptorSets;
    };

};  // namespace mat
