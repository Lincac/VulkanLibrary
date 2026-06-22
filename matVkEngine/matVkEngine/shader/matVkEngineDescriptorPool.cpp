#include "matVkEngineDescriptorPool.h"

#include <stdexcept>

namespace mat {

    VkEngineDescriptorPool::VkEngineDescriptorPool() {}

    VkEngineDescriptorPool::~VkEngineDescriptorPool() {}

    VkDescriptorPool VkEngineDescriptorPool::getVkDescriptorPool() const {
        return _descriptorPool;
    }

    void VkEngineDescriptorPool::create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                        std::vector<std::shared_ptr<VkEngineDescriptorSetLayout>> layouts) {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (const auto& layout : layouts) {
            for (const auto& bind : layout->getBindings()) {
                bool found = false;
                for (auto& poolSize : poolSizes) {
                    if (poolSize.type == bind.descriptorType) {
                        poolSize.descriptorCount += bind.descriptorCount;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    VkDescriptorPoolSize poolSize{};
                    poolSize.type = bind.descriptorType;
                    poolSize.descriptorCount = bind.descriptorCount;
                    poolSizes.push_back(poolSize);
                }
            }
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = layouts.size() * MAX_FRAMES_IN_FLIGHT;

        if (vkCreateDescriptorPool(logicalDevice->getVkDevice(), &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void VkEngineDescriptorPool::release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(logicalDevice->getVkDevice(), _descriptorPool, nullptr);
            _descriptorPool = VK_NULL_HANDLE;
        }
    }

};  // namespace mat