#include "vkEngineAccelerationStructure.h"

vkEngineAccelerationStructure::vkEngineAccelerationStructure(std::shared_ptr<vkEngineLogicalDevice> device, Type type)
    : _device(device)    
{
    _type = type;
    _asBuffer = std::make_shared<vkEngineBuffer>(device);
    _scratchBuffer = std::make_shared<vkEngineBuffer>(device);
}

vkEngineAccelerationStructure::~vkEngineAccelerationStructure()
{
    if (_handle != VK_NULL_HANDLE) {
        vkDestroyAccelerationStructureKHR(_device->getVkDevice(), _handle, nullptr);
        _handle = VK_NULL_HANDLE;
    }
}

void vkEngineAccelerationStructure::setTriangleGeometry(VkDeviceAddress vertexAddress, uint32_t vertexCount)
{
    _vertexAddress = vertexAddress;
    _vertexCount = vertexCount;
}

void vkEngineAccelerationStructure::build(std::shared_ptr<vkEngineCommandPool> commandPool)
{
    // Step 1 — 填 geometry
    VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress = _vertexAddress;
    triangles.vertexStride = sizeof(float) * 3;   // 或 sizeof(Vertex)
    triangles.maxVertex = _vertexCount - 1;
    triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles = triangles;

    // Step 2 — 查询 build 大小
    uint32_t maxPrimitiveCount = 1;  // 1 个三角形

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR sizes{};
    sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetAccelerationStructureBuildSizesKHR(
        _device->getVkDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &maxPrimitiveCount,
        &sizes);

    // Step 3 — 创建 AS buffer 和 scratch buffer
    _asBuffer->setSize(sizes.accelerationStructureSize);
    _asBuffer->setUsage(
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

    _asBuffer->setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _asBuffer->create();     

    // Scratch buffer（build 完不再需要，但先存着）：
    _scratchBuffer->setSize(sizes.buildScratchSize);
    _scratchBuffer->setUsage(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    _scratchBuffer->setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _scratchBuffer->create();
    
    // Step 4 — 创建 AS 对象
    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = _asBuffer->getBuffer();
    createInfo.offset = 0;
    createInfo.size = sizes.accelerationStructureSize;
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    vkCreateAccelerationStructureKHR(_device->getVkDevice(), &createInfo, nullptr, &_handle);
    
    // Step 5 — 提交 build 命令
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure = _handle;
    buildInfo.scratchData.deviceAddress = _scratchBuffer->getDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount = 1;
    rangeInfo.primitiveOffset = 0;
    rangeInfo.firstVertex = 0;
    rangeInfo.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);
    });
}

VkAccelerationStructureKHR &vkEngineAccelerationStructure::getHandle()
{
    return _handle;
}

VkDeviceAddress vkEngineAccelerationStructure::getDeviceAddress()
{
    VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
    addrInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addrInfo.accelerationStructure = _handle;
    
    return vkGetAccelerationStructureDeviceAddressKHR(_device->getVkDevice(), &addrInfo);
}
