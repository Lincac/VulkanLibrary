#include "vkEngineImage.h"
#include "vkEngineAccelerationStructure.h"
#include "vkEngineRTDescriptor.h"
#include "vkEngineRayTracingPipeline.h"
#include "vkEngineHelp.h"

#include <iostream>

int main(){
    auto engine = std::make_shared<vkEngine>("Vulkan Demo", true);

    auto physicalDevice = std::make_shared<vkEnginePhysicalDevice>(engine);
    auto logicalDevice = std::make_shared<vkEngineLogicalDevice>(physicalDevice);
    auto commandPool = std::make_shared<vkEngineCommandPool>(logicalDevice);
    std::cout << "vulkan is init" << std::endl;

    auto image = std::make_shared<vkEngineImage>(logicalDevice);
    image->setResolution(glm::ivec2(800,600));
    image->setFormat(VK_FORMAT_R8G8B8A8_UNORM);
    image->setImageUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    image->generate();
    std::cout << "storage image ready" << std::endl;

    // 提交一次命令，将图像从 UNDEFINED 状态转换为 GENERAL 状态
    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        transitionImageLayout(cmd, image->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    });

    struct Vertex { float pos[3]; };

    const std::vector<Vertex> triangleVertices = {
        {{-2.0f, -2.0f, 0.0f }},
        {{ 0.0f,  2.0f, 0.0f }},
        {{ 2.0f, -2.0f, 0.0f }},
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
    vertexBuffer.upload(*commandPool.get(), triangleVertices.data(),
        sizeof(Vertex) * triangleVertices.size());
    std::cout << "vertex buffer ready, address = "
            << vertexBuffer.getDeviceAddress() << std::endl;    

    vkEngineAccelerationStructure blas(logicalDevice, vkEngineAccelerationStructure::Type::BLAS);
    blas.setTriangleGeometry(vertexBuffer.getDeviceAddress(), 3);
    blas.build(*commandPool.get());
    std::cout << "BLAS ready, address = " << blas.getDeviceAddress() << std::endl;

    vkEngineAccelerationStructure tlas(logicalDevice, vkEngineAccelerationStructure::Type::TLAS);
    tlas.setInstance(blas);   // 默认单位矩阵
    tlas.build(*commandPool.get());
    std::cout << "TLAS ready, address = " << tlas.getDeviceAddress() << std::endl;

    vkEngineRTDescriptor descriptor(logicalDevice);
    descriptor.create();
    descriptor.setup(tlas, *image);
    std::cout << "descriptor ready" << std::endl;

    vkEngineRayTracingPipeline rtPipeline(logicalDevice);
    rtPipeline.setShaders("shaders/raygen.spv", "shaders/miss.spv", "shaders/closesthit.spv");
    rtPipeline.setDescriptorSetLayout(descriptor.getLayout());
    rtPipeline.create();
    rtPipeline.createSBT();
    std::cout << "RT pipeline ready" << std::endl;

    const std::string outputPath = pathNextToExe("output.png");
    image->saveToPng(*commandPool.get(), outputPath, [&](VkCommandBuffer cmd) {
        rtPipeline.recordTrace(cmd, descriptor.getSet(), 800, 600, 1);
    });
    std::cout << "trace rays done" << std::endl;
    std::cout << "saved " << outputPath << std::endl;

    return 0;
}