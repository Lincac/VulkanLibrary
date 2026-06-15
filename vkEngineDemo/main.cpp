#include "core/vkEngineContext.h"
#include "device/vkEnginePhysicalDevice.h"
#include "device/vkEngineLogicalDevice.h"
#include "device/vkEngineCommandPool.h"
#include "resource/vkEngineBuffer.h"
#include "resource/vkEngineImage.h"
#include "resource/vkEngineTexture.h"
#include "rt/vkEngineAccelerationStructure.h"
#include "rt/vkEngineRTDescriptor.h"
#include "rt/vkEngineRayTracingPipeline.h"
#include "rt/vkEngineRTHelp.h"

#include <iostream>

int main(){
    vkEngineConfig config;
    config.applicationName = "Vulkan Demo";
    config.enableValidationLayers = true;
    auto context = std::make_shared<vkEngineContext>(config);

    auto physicalDevice = std::make_shared<vkEnginePhysicalDevice>(context);
    auto logicalDevice = std::make_shared<vkEngineLogicalDevice>(physicalDevice);
    auto commandPool = std::make_shared<vkEngineCommandPool>(logicalDevice);

    auto image = std::make_shared<vkEngineImage>(logicalDevice);
    image->setResolution(glm::ivec2(1280,720));
    image->setFormat(VK_FORMAT_R8G8B8A8_UNORM);
    image->setImageUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    image->generate();

    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        transitionImageLayout(cmd, image->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    });

    auto environmentMap = std::make_shared<vkEngineTexture>(logicalDevice);
    environmentMap->loadHdr(commandPool, "hdr/kloofendal_48d_partly_cloudy_puresky_4k.hdr");
    std::cout << "environment HDR ready " << environmentMap->getResolution().x
              << "x" << environmentMap->getResolution().y << std::endl;
    std::cout << "environment CDF ready " << environmentMap->getEnvCdfResolution().x
              << "x" << environmentMap->getEnvCdfResolution().y << std::endl;

    const ObjMesh mesh = loadObj("models/sphere.obj");
    std::cout << "loaded obj: " << mesh.triangleCount() << " triangles, "
              << mesh.vertices.size() << " vertices" << std::endl;

    auto vertexBuffer = std::make_shared<vkEngineBuffer>(logicalDevice);
    vertexBuffer->setSize(sizeof(ObjVertex) * mesh.vertices.size());
    vertexBuffer->setUsage(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vertexBuffer->setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffer->create();
    vertexBuffer->upload(commandPool, mesh.vertices.data(),
        sizeof(ObjVertex) * mesh.vertices.size());

    auto blas = std::make_shared<vkEngineAccelerationStructure>(logicalDevice, vkEngineAccelerationStructure::Type::BLAS);
    blas->setTriangleGeometry(vertexBuffer->getDeviceAddress(),
        static_cast<uint32_t>(mesh.vertices.size()),
        static_cast<uint32_t>(sizeof(ObjVertex)));
    blas->build(commandPool);

    auto tlas = std::make_shared<vkEngineAccelerationStructure>(logicalDevice, vkEngineAccelerationStructure::Type::TLAS);
    tlas->setInstance(blas);
    tlas->build(commandPool);

    PathTraceSettingsGPU pathTraceSettings{};
    pathTraceSettings.exposure = -0.3;
    auto settingsBuffer = std::make_shared<vkEngineBuffer>(logicalDevice);
    settingsBuffer->setSize(sizeof(PathTraceSettingsGPU));
    settingsBuffer->setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    settingsBuffer->setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    settingsBuffer->create();
    settingsBuffer->upload(commandPool, &pathTraceSettings, sizeof(PathTraceSettingsGPU));

    vkEngineRTDescriptor descriptor(logicalDevice);
    descriptor.create();
    descriptor.setup(tlas, image, vertexBuffer, environmentMap, settingsBuffer);

    vkEngineRayTracingPipeline rtPipeline(logicalDevice);
    rtPipeline.setShaders("shaders/raygen.spv", "shaders/miss.spv", "shaders/closesthit.spv");
    rtPipeline.setDescriptorSetLayout(descriptor.getLayout());
    rtPipeline.create();
    rtPipeline.createSBT();

    const std::string outputPath = pathNextToExe("output.png");
    image->saveToPng(commandPool, outputPath, [&](VkCommandBuffer cmd) {
        rtPipeline.recordTrace(cmd, descriptor.getSet(), 1280, 720, 1);
    });
    std::cout << "path trace done" << std::endl;
    std::cout << "saved " << outputPath << std::endl;

    return 0;
}
