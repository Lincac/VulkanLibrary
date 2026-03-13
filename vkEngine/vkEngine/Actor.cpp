#include "Actor.h"

Actor& Actor::setVertexShader(const std::string& shaderPath)
{
    _vertexShaderPath = shaderPath;
    return *this;
}

Actor& Actor::setFragmentShader(const std::string& shaderPath)
{
    _fragmentShaderPath = shaderPath;
    return *this;
}

Actor& Actor::setVertexInput(
    const std::vector<VkVertexInputBindingDescription>& bindings,
    const std::vector<VkVertexInputAttributeDescription>& attributes)
{
    _bindings = bindings;
    _attributes = attributes;
    return *this;
}

Actor& Actor::setVertexBuffer(VkBuffer vertexBuffer, VkDeviceSize offset)
{
    _vertexBuffer = vertexBuffer;
    _vertexOffset = offset;
    return *this;
}

Actor& Actor::setIndexBuffer(VkBuffer indexBuffer, uint32_t indexCount, VkIndexType indexType)
{
    _indexBuffer = indexBuffer;
    _indexCount = indexCount;
    _indexType = indexType;
    _useIndexedDraw = indexBuffer != VK_NULL_HANDLE && indexCount > 0;
    return *this;
}

Actor& Actor::setDrawParams(uint32_t vertexCount, uint32_t instanceCount)
{
    _vertexCount = vertexCount;
    _instanceCount = instanceCount;
    return *this;
}

const std::string& Actor::getVertexShader() const noexcept
{
    return _vertexShaderPath;
}

const std::string& Actor::getFragmentShader() const noexcept
{
    return _fragmentShaderPath;
}

const std::vector<VkVertexInputBindingDescription>& Actor::getBindings() const noexcept
{
    return _bindings;
}

const std::vector<VkVertexInputAttributeDescription>& Actor::getAttributes() const noexcept
{
    return _attributes;
}

VkBuffer Actor::getVertexBuffer() const noexcept
{
    return _vertexBuffer;
}

VkDeviceSize Actor::getVertexOffset() const noexcept
{
    return _vertexOffset;
}

VkBuffer Actor::getIndexBuffer() const noexcept
{
    return _indexBuffer;
}

uint32_t Actor::getIndexCount() const noexcept
{
    return _indexCount;
}

VkIndexType Actor::getIndexType() const noexcept
{
    return _indexType;
}

bool Actor::useIndexedDraw() const noexcept
{
    return _useIndexedDraw;
}

uint32_t Actor::getVertexCount() const noexcept
{
    return _vertexCount;
}

uint32_t Actor::getInstanceCount() const noexcept
{
    return _instanceCount;
}
