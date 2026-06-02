#include "vkEngineBuffer.h"
#include "vkEngineImage.h"

#include <iostream>

int main(){
    auto engine = std::make_shared<vkEngine>("Vulkan Demo", true);

    auto physicalDevice = std::make_shared<vkEnginePhysicalDevice>(engine);
    auto logicalDevice = std::make_shared<vkEngineLogicalDevice>(physicalDevice);
    auto commandPool = std::make_shared<vkEngineCommandPool>(logicalDevice);

    auto image = std::make_shared<vkEngineImage>(logicalDevice);
    image->setResolution(glm::ivec2(800,600));
    image->setFormat(VK_FORMAT_R8G8B8A8_UNORM);
    image->setImageUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    image->generate();

    // 提交一次命令，将图像从 UNDEFINED 状态转换为 GENERAL 状态
    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        transitionImageLayout(cmd, image->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    });

    struct Vertex { float pos[3]; };

    const std::vector<Vertex> triangleVertices = {
        {{ 0.0f, -0.5f, 0.0f }},
        {{ 0.5f,  0.5f, 0.0f }},
        {{-0.5f,  0.5f, 0.0f }},
    };

    // 在 commandPool 创建之后：
    vkEngineBuffer vertexBuffer(logicalDevice);
    vertexBuffer.setSize(sizeof(Vertex) * triangleVertices.size());
    vertexBuffer.setUsage(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vertexBuffer.setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer.create();
    vertexBuffer.upload(commandPool, triangleVertices.data(),
        sizeof(Vertex) * triangleVertices.size());
        
    std::cout << "vertex buffer ready, address = "
            << vertexBuffer.getDeviceAddress() << std::endl;    

    std::cout << "storage image ready" << std::endl;
    std::cout << "vulkan is init" << std::endl;

    return 0;
}