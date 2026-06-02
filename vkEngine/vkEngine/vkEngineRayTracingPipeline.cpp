#include "vkEngineRayTracingPipeline.h"
#include "vkEngineHelp.h"

#include <cstring>
#include <stdexcept>

vkEngineRayTracingPipeline::vkEngineRayTracingPipeline(std::shared_ptr<vkEngineLogicalDevice> device)
    : _device(device)
{
}

vkEngineRayTracingPipeline::~vkEngineRayTracingPipeline()
{
    auto device = _device->getVkDevice();

    if (_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }

    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }

    for (VkShaderModule module : _shaderModules) {
        vkDestroyShaderModule(device, module, nullptr);
    }
    
    _shaderModules.clear();
}

void vkEngineRayTracingPipeline::setShaders(const std::string& raygen, const std::string& miss, const std::string& closestHit)
{
    _raygenPath = raygen;
    _missPath = miss;
    _closestHitPath = closestHit;
}

void vkEngineRayTracingPipeline::setDescriptorSetLayout(VkDescriptorSetLayout layout)
{
    _descriptorSetLayout = layout;
}

VkShaderModule vkEngineRayTracingPipeline::createShaderModule(const std::vector<char>& code)
{
    if (code.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error("shader code size is not a multiple of 4");
    }

    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = code.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(_device->getVkDevice(), &moduleInfo, nullptr, &module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    _shaderModules.push_back(module);
    return module;
}

void vkEngineRayTracingPipeline::create()
{
    if (_raygenPath.empty() || _missPath.empty() || _closestHitPath.empty()) {
        throw std::runtime_error("shader paths not set, call setShaders() first");
    }

    if (_descriptorSetLayout == VK_NULL_HANDLE) {
        throw std::runtime_error("descriptor set layout not set");
    }

    auto device = _device->getVkDevice();

    VkShaderModule raygenModule = createShaderModule(readFile(resolvePathNextToExe(_raygenPath)));
    VkShaderModule missModule = createShaderModule(readFile(resolvePathNextToExe(_missPath)));
    VkShaderModule closestHitModule = createShaderModule(readFile(resolvePathNextToExe(_closestHitPath)));

    VkPipelineShaderStageCreateInfo stages[3]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stages[0].module = raygenModule;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages[1].module = missModule;
    stages[1].pName = "main";

    stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages[2].module = closestHitModule;
    stages[2].pName = "main";

    VkRayTracingShaderGroupCreateInfoKHR groups[3]{};

    groups[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[0].generalShader = 0;
    groups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

    groups[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[1].generalShader = 1;
    groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

    groups[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    groups[2].generalShader = VK_SHADER_UNUSED_KHR;
    groups[2].closestHitShader = 2;
    groups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &_descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = 3;
    pipelineInfo.pStages = stages;
    pipelineInfo.groupCount = 3;
    pipelineInfo.pGroups = groups;
    pipelineInfo.maxPipelineRayRecursionDepth = 1;
    pipelineInfo.layout = _pipelineLayout;

    if (vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ray tracing pipeline!");
    }
}

void vkEngineRayTracingPipeline::createSBT()
{
    if (_pipeline == VK_NULL_HANDLE) {
        throw std::runtime_error("pipeline not created, call create() first");
    }

    auto physDev = _device->getPhysicalDevice()->getVkPhysicalDevice();
    auto device = _device->getVkDevice();

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
    };
    VkPhysicalDeviceProperties2 props2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    props2.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(physDev, &props2);

    _shaderGroupHandleSize = rtProps.shaderGroupHandleSize;
    _shaderGroupHandleAlignment = rtProps.shaderGroupBaseAlignment;

    std::vector<uint8_t> shaderHandles(_shaderGroupCount * _shaderGroupHandleSize);
    if (vkGetRayTracingShaderGroupHandlesKHR(
            device, _pipeline, 0, _shaderGroupCount,
            static_cast<size_t>(_shaderGroupHandleSize) * _shaderGroupCount,
            shaderHandles.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to get ray tracing shader group handles!");
    }

    const VkDeviceSize sbtSize = _shaderGroupCount * _shaderGroupHandleAlignment;

    _sbtBuffer = std::make_shared<vkEngineBuffer>(_device);
    _sbtBuffer->setSize(sbtSize);
    _sbtBuffer->setUsage(
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    _sbtBuffer->setMemoryProperties(
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    _sbtBuffer->create();

    void* mapped = nullptr;
    vkMapMemory(device, _sbtBuffer->getMemory(), 0, sbtSize, 0, &mapped);
    for (uint32_t i = 0; i < _shaderGroupCount; ++i) {
        std::memcpy(
            static_cast<uint8_t*>(mapped) + i * _shaderGroupHandleAlignment,
            shaderHandles.data() + i * _shaderGroupHandleSize,
            _shaderGroupHandleSize);
    }
    vkUnmapMemory(device, _sbtBuffer->getMemory());

    const VkDeviceAddress sbtAddress = _sbtBuffer->getDeviceAddress();

    _raygenRegion.deviceAddress = sbtAddress;
    _raygenRegion.stride = _shaderGroupHandleAlignment;
    _raygenRegion.size = _shaderGroupHandleAlignment;

    _missRegion.deviceAddress = sbtAddress + _shaderGroupHandleAlignment;
    _missRegion.stride = _shaderGroupHandleAlignment;
    _missRegion.size = _shaderGroupHandleAlignment;

    _hitRegion.deviceAddress = sbtAddress + 2 * _shaderGroupHandleAlignment;
    _hitRegion.stride = _shaderGroupHandleAlignment;
    _hitRegion.size = _shaderGroupHandleAlignment;

    _callableRegion = {};
}

void vkEngineRayTracingPipeline::recordTrace(VkCommandBuffer cmd, VkDescriptorSet descriptorSet,
    uint32_t width, uint32_t height, uint32_t depth)
{
    if (_pipeline == VK_NULL_HANDLE || _sbtBuffer == nullptr) {
        throw std::runtime_error("pipeline/SBT not ready, call create() and createSBT() first");
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        _pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdTraceRaysKHR(cmd,
        &_raygenRegion, &_missRegion, &_hitRegion, &_callableRegion,
        width, height, depth);
}

VkPipeline& vkEngineRayTracingPipeline::getPipeline()
{
    return _pipeline;
}

VkPipelineLayout& vkEngineRayTracingPipeline::getPipelineLayout()
{
    return _pipelineLayout;
}

const VkStridedDeviceAddressRegionKHR& vkEngineRayTracingPipeline::getRaygenRegion() const
{
    return _raygenRegion;
}

const VkStridedDeviceAddressRegionKHR& vkEngineRayTracingPipeline::getMissRegion() const
{
    return _missRegion;
}

const VkStridedDeviceAddressRegionKHR& vkEngineRayTracingPipeline::getHitRegion() const
{
    return _hitRegion;
}

const VkStridedDeviceAddressRegionKHR& vkEngineRayTracingPipeline::getCallableRegion() const
{
    return _callableRegion;
}
