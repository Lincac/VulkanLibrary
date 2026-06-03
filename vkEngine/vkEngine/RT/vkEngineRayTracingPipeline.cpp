#include "vkEngineRayTracingPipeline.h"
#include "vkEngineHelp.h"

#include <cstring>
#include <stdexcept>

namespace {

VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

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

    // 从 exe 同目录加载 SPIR-V，创建 VkShaderModule（驱动内部持有 shader 字节码，非显式 VkBuffer）
    VkShaderModule raygenModule = createShaderModule(readFile(resolvePathNextToExe(_raygenPath)));
    VkShaderModule missModule = createShaderModule(readFile(resolvePathNextToExe(_missPath)));
    VkShaderModule closestHitModule = createShaderModule(readFile(resolvePathNextToExe(_closestHitPath)));

    // 注册 RT 管线中的 3 个 shader stage；groups[] 通过下标引用此数组
    VkPipelineShaderStageCreateInfo stages[3]{};

    // stages[0]: Ray Generation — vkCmdTraceRaysKHR 启动时执行的入口 shader
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; // 结构体类型标识
    stages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;                      // shader 在 RT 管线中的角色
    stages[0].module = raygenModule;                                         // 关联的 VkShaderModule
    stages[0].pName = "main";                                                // SPIR-V 入口函数名（GLSL 的 main）

    // stages[1]: Miss — 光线未命中几何时执行
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages[1].module = missModule;
    stages[1].pName = "main";

    // stages[2]: Closest Hit — 光线命中三角形时执行
    stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages[2].module = closestHitModule;
    stages[2].pName = "main";

    // 将 stages[] 中的 shader 打包为 group，供 SBT 与 traceRayEXT 按索引引用。
    // generalShader / closestHitShader 等字段的值是 stages 数组下标，不是 group 下标。
    VkRayTracingShaderGroupCreateInfoKHR groups[3]{};

    // Group 0: Ray Generation — vkCmdTraceRaysKHR 的 raygen region 绑定此组
    groups[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[0].generalShader = 0; // stages[0] raygen
    groups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

    // Group 1: Miss — 光线未命中几何体时执行；traceRayEXT 的 missIndex 指向 miss SBT 中的记录
    groups[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[1].generalShader = 1; // stages[1] miss
    groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

    // Group 2: Triangle Hit Group — 命中三角形时执行 closest hit；无需自定义 intersection shader
    groups[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    groups[2].generalShader = VK_SHADER_UNUSED_KHR;
    groups[2].closestHitShader = 2; // stages[2] closest hit
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
    pipelineInfo.maxPipelineRayRecursionDepth = 2;
    pipelineInfo.layout = _pipelineLayout;

    // 创建 RT 管线（把 stage 编进 pipeline）
    if (vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ray tracing pipeline!");
    }

    vkDeviceWaitIdle(device);
}

void vkEngineRayTracingPipeline::createSBT()
{
    if (_pipeline == VK_NULL_HANDLE) {
        throw std::runtime_error("pipeline not created, call create() first");
    }

    auto physDev = _device->getPhysicalDevice()->getVkPhysicalDevice();
    auto device = _device->getVkDevice();

    // 通过 pNext 链查询 RT 管线相关硬件属性（handle 大小、对齐等），用于后续 SBT 布局
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
    };
    VkPhysicalDeviceProperties2 props2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    props2.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(physDev, &props2);

    // 每个 shader group 在 SBT 里占 32 字节（NVIDIA 常见值）。
    _shaderGroupHandleSize = rtProps.shaderGroupHandleSize;
    // SBT 起始地址需 64 字节对齐
    _shaderGroupHandleAlignment = rtProps.shaderGroupHandleAlignment;

    const VkDeviceSize handleSize = _shaderGroupHandleSize;
    const VkDeviceSize handleSizeAligned = alignUp(handleSize, _shaderGroupHandleAlignment);
    const VkDeviceSize baseAlign = rtProps.shaderGroupBaseAlignment;
    const VkDeviceSize recordSize = alignUp(handleSizeAligned, baseAlign);

    // 计算 SBT 中存储所有 shader handle 所需的总大小
    const VkDeviceSize handleStorageSize = _shaderGroupCount * handleSizeAligned;
    std::vector<uint8_t> shaderHandles(handleStorageSize);
    if (vkGetRayTracingShaderGroupHandlesKHR(
            device, _pipeline, 0, _shaderGroupCount,
            static_cast<size_t>(handleStorageSize),
            shaderHandles.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to get ray tracing shader group handles!");
    }

    _sbtBuffers.clear();

    auto createRegionBuffer = [&](const uint8_t* handleData) {
        const VkDeviceSize bufferSize = recordSize + baseAlign;
        auto buffer = std::make_shared<vkEngineBuffer>(_device);
        buffer->setSize(bufferSize);
        buffer->setUsage(
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        buffer->setMemoryProperties(
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        buffer->create();
        _sbtBuffers.push_back(buffer);

        const VkDeviceAddress baseAddress = buffer->getDeviceAddress();
        const VkDeviceAddress alignedAddress = alignUp(baseAddress, baseAlign);
        const VkDeviceSize sbtOffset = alignedAddress - baseAddress;

        void* mapped = nullptr;
        vkMapMemory(device, buffer->getMemory(), 0, bufferSize, 0, &mapped);
        std::memset(mapped, 0, static_cast<size_t>(bufferSize));
        std::memcpy(static_cast<uint8_t*>(mapped) + sbtOffset, handleData, static_cast<size_t>(handleSize));
        vkUnmapMemory(device, buffer->getMemory());

        // 创建 region，用于绑定 SBT 到 pipeline
        VkStridedDeviceAddressRegionKHR region{};
        region.deviceAddress = alignedAddress; // 起始地址
        region.stride = recordSize; // 步长
        region.size = recordSize; // 大小
        return region;
    };

    _raygenRegion = createRegionBuffer(shaderHandles.data());
    _missRegion = createRegionBuffer(shaderHandles.data() + handleSizeAligned);
    _hitRegion = createRegionBuffer(shaderHandles.data() + 2 * handleSizeAligned);
    _callableRegion = {};
}

void vkEngineRayTracingPipeline::recordTrace(VkCommandBuffer cmd, VkDescriptorSet descriptorSet,
    uint32_t width, uint32_t height, uint32_t depth)
{
    if (_pipeline == VK_NULL_HANDLE || _sbtBuffers.empty()) {
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
