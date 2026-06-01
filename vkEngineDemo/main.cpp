#include "vkEngineCommandPool.h"
#include "vkEngineImage.h"

#include <iostream>

int main(){
    vkEngine engine("Vulkan Demo", true);

    vkEnginePhysicalDevice physicalDevice(engine);
    vkEngineLogicalDevice logicalDevice(physicalDevice);
    vkEngineCommandPool commandPool(logicalDevice);

    vkEngineImage image(logicalDevice);
    image.setResolution(glm::ivec2(800,600));
    image.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
    image.setImageUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    image.generate();

    // 1. 创建 fence
    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    vkCreateFence(logicalDevice.getVkDevice(), &fenceInfo, nullptr, &fence);

    // 2. 分配 one-time command buffer（临时 command pool 也行）
    VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = physicalDevice.findQueueFamilies().graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VkCommandPool oneTimePool;
    vkCreateCommandPool(logicalDevice.getVkDevice(), &poolInfo, nullptr, &oneTimePool);

    VkCommandBufferAllocateInfo cmdAlloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdAlloc.commandPool = oneTimePool;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(logicalDevice.getVkDevice(), &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    transitionImageLayout(cmd, image.getImage(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    vkEndCommandBuffer(cmd);

    // 3. submit
    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(logicalDevice.getGraphicsQueue(), 1, &submit, fence);
    vkWaitForFences(logicalDevice.getVkDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

    std::cout << "storage image ready" << std::endl;

    std::cout << "vulkan is init" << std::endl;

    return 0;
}