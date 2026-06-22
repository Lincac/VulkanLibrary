#pragma once

#include "shader/matVkEngineDescriptorSetLayout.h"

namespace mat {

    class VkEngineDescriptorPool {
    public:
        VkEngineDescriptorPool();
        ~VkEngineDescriptorPool();

        VkDescriptorPool getVkDescriptorPool() const;

        void create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                    std::vector<std::shared_ptr<VkEngineDescriptorSetLayout>> layouts);

        void release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

    private:
        VkEngineDescriptorPool(const VkEngineDescriptorPool&) = delete;
        VkEngineDescriptorPool(VkEngineDescriptorPool&&) = delete;
        VkEngineDescriptorPool& operator=(const VkEngineDescriptorPool&) = delete;
        VkEngineDescriptorPool& operator=(VkEngineDescriptorPool&&) = delete;

        VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
    };

};  // namespace mat