#include <stdexcept>
#include <iostream>

#include "common/matVkEngineCommon.h"
#include "common/matVkEngineMesh.h"
#include "device/matVkEngineCmdPool.h"
#include "resource/matVkEngineImage.h"
#include "resource/matVkEngineBuffer.h"

int main() {
    if (volkInitialize() != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    auto context = std::make_shared<mat::VkEngineContext>();
    auto physicalDevice = std::make_shared<mat::VkEnginePhysicalDevice>(context);
    auto logicalDevice = std::make_shared<mat::VkEngineLogicalDevice>(physicalDevice);
    auto cmd = std::make_shared<mat::VkEngineCmdPool>(physicalDevice, logicalDevice);

    auto image = std::make_shared<mat::VkEngineImage>();
    image->setResolution(1280, 720);
    image->setVkFormat(VK_FORMAT_R8G8B8A8_UNORM);
    image->setVkImageUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    image->create(physicalDevice, logicalDevice);

    cmd->submitOneTimeCommands(logicalDevice, [&](VkCommandBuffer cmd) {
        mat::transitionImageLayout(cmd, image->getVkImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                   mat::ImageShaderDomain::Compute);
    });

    auto mesh = std::make_shared<mat::VkEngineMesh>();
    mesh->load("models/scene.obj");
    std::cerr << "mesh vertices num: " << mesh->getVertices().size() << std::endl;

    auto vertexBuffer = std::make_shared<mat::VkEngineBuffer>();
    vertexBuffer->setVkDeviceSize(sizeof(mat::Vertex) * mesh->getVertices().size());
    vertexBuffer->setVkBufferUsageFlags(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vertexBuffer->setVkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer->create(physicalDevice, logicalDevice);
    vertexBuffer->upload(physicalDevice, logicalDevice, cmd, mesh->getVertices().data(),
                         sizeof(mat::Vertex) * mesh->getVertices().size());

    vertexBuffer->release(logicalDevice);
    image->release(logicalDevice);
    cmd->release(logicalDevice);

    return 0;
}