#include "vkEngineAccelerationStructure.h"

namespace {

// 与 VkAccelerationStructureInstanceKHR 设备端布局一致（64 字节）
struct DeviceAccelerationStructureInstance {
    VkTransformMatrixKHR transform; // 变换矩阵
    uint32_t instanceCustomIndexAndMask; // 实例自定义索引和掩码
    uint32_t instanceShaderBindingTableRecordOffsetAndFlags; // 实例着色器绑定表记录偏移和标志
    uint64_t accelerationStructureReference; // 加速结构体引用
};

// 断言 DeviceAccelerationStructureInstance 布局与 VkAccelerationStructureInstanceKHR 一致（64 字节）
static_assert(sizeof(DeviceAccelerationStructureInstance) == 64,
    "DeviceAccelerationStructureInstance layout mismatch");

} // namespace

vkEngineAccelerationStructure::vkEngineAccelerationStructure(std::shared_ptr<vkEngineLogicalDevice> device, Type type)
    : _device(device)
    , _type(type)
{
    _asBuffer = std::make_shared<vkEngineBuffer>(device);
    _scratchBuffer = std::make_shared<vkEngineBuffer>(device);
    _instanceBuffer = std::make_shared<vkEngineBuffer>(device);
}

vkEngineAccelerationStructure::~vkEngineAccelerationStructure()
{
    if (_handle != VK_NULL_HANDLE) {
        vkDestroyAccelerationStructureKHR(_device->getVkDevice(), _handle, nullptr);
        _handle = VK_NULL_HANDLE;
    }
}

void vkEngineAccelerationStructure::setTriangleGeometry(VkDeviceAddress vertexAddress, uint32_t vertexCount,
    uint32_t vertexStride)
{
    if (_type != Type::BLAS) {
        throw std::runtime_error("setTriangleGeometry() is only valid for BLAS");
    }

    _vertexAddress = vertexAddress;
    _vertexCount = vertexCount;
    _vertexStride = vertexStride;
}

void vkEngineAccelerationStructure::setInstance(std::shared_ptr<vkEngineAccelerationStructure> blas, const glm::mat4& transform)
{
    if (_type != Type::TLAS) {
        throw std::runtime_error("setInstance() is only valid for TLAS");
    }

    if (blas->_type != Type::BLAS) {
        throw std::runtime_error("setInstance() requires a BLAS");
    }

    DeviceAccelerationStructureInstance instance{};
    instance.transform = toVkTransform(transform);
    instance.instanceCustomIndexAndMask = (0xFFu << 24);
    instance.instanceShaderBindingTableRecordOffsetAndFlags =
        (static_cast<uint32_t>(VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR) << 24);
    instance.accelerationStructureReference = blas->getDeviceAddress();

    const auto* bytes = reinterpret_cast<const uint8_t*>(&instance);
    _instanceData.insert(_instanceData.end(), bytes, bytes + sizeof(instance));
}

void vkEngineAccelerationStructure::build(std::shared_ptr<vkEngineCommandPool> commandPool)
{
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    VkAccelerationStructureTypeKHR asType{};
    uint32_t maxPrimitiveCount = 0;

    if (_type == Type::BLAS) {
        if (_vertexCount == 0 || _vertexAddress == 0) {
            throw std::runtime_error("BLAS geometry not set, call setTriangleGeometry() first");
        }
        if (_vertexCount % 3 != 0) {
            throw std::runtime_error("BLAS vertex count must be a multiple of 3 (triangle list)");
        }

        VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = _vertexAddress;
        triangles.vertexStride = _vertexStride;
        triangles.maxVertex = _vertexCount - 1;
        triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        triangles.indexData.deviceAddress = 0;
        triangles.transformData.deviceAddress = 0;

        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles = triangles;

        asType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        maxPrimitiveCount = _vertexCount / 3;
    }
    else {
        if (_instanceData.empty()) {
            throw std::runtime_error("TLAS instances not set, call setInstance() first");
        }

        const VkDeviceSize instanceBufferSize = _instanceData.size();

        _instanceBuffer->setSize(instanceBufferSize);
        _instanceBuffer->setUsage(
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        _instanceBuffer->setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        _instanceBuffer->create();
        _instanceBuffer->upload(commandPool, _instanceData.data(), instanceBufferSize);

        VkAccelerationStructureGeometryInstancesDataKHR instances{};
        instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        instances.arrayOfPointers = VK_FALSE;
        instances.data.deviceAddress = _instanceBuffer->getDeviceAddress();

        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances = instances;

        asType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        maxPrimitiveCount = static_cast<uint32_t>(_instanceData.size() / sizeof(DeviceAccelerationStructureInstance));
    }

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = asType;
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

    _asBuffer->setSize(sizes.accelerationStructureSize);
    _asBuffer->setUsage(
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    _asBuffer->setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _asBuffer->create();

    _scratchBuffer->setSize(sizes.buildScratchSize);
    _scratchBuffer->setUsage(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    _scratchBuffer->setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _scratchBuffer->create();

    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = _asBuffer->getBuffer();
    createInfo.offset = 0;
    createInfo.size = sizes.accelerationStructureSize;
    createInfo.type = asType;

    if (vkCreateAccelerationStructureKHR(_device->getVkDevice(), &createInfo, nullptr, &_handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create acceleration structure!");
    }

    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure = _handle;
    buildInfo.scratchData.deviceAddress = _scratchBuffer->getDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount = maxPrimitiveCount;
    rangeInfo.primitiveOffset = 0;
    rangeInfo.firstVertex = 0;
    rangeInfo.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);
    });
}

VkAccelerationStructureKHR& vkEngineAccelerationStructure::getHandle()
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
