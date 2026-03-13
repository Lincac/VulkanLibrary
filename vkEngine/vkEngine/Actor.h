#pragma once

#include "vkEngine.h"

class Actor
{
public:
    Actor() = default;

    Actor& setVertexShader(const std::string& shaderPath);
    Actor& setFragmentShader(const std::string& shaderPath);
    Actor& setVertexInput(
        const std::vector<VkVertexInputBindingDescription>& bindings,
        const std::vector<VkVertexInputAttributeDescription>& attributes);
    Actor& setVertexBuffer(VkBuffer vertexBuffer, VkDeviceSize offset = 0);
    Actor& setIndexBuffer(VkBuffer indexBuffer, uint32_t indexCount, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    Actor& setDrawParams(uint32_t vertexCount, uint32_t instanceCount = 1);

    const std::string& getVertexShader() const noexcept;
    const std::string& getFragmentShader() const noexcept;
    const std::vector<VkVertexInputBindingDescription>& getBindings() const noexcept;
    const std::vector<VkVertexInputAttributeDescription>& getAttributes() const noexcept;
    VkBuffer getVertexBuffer() const noexcept;
    VkDeviceSize getVertexOffset() const noexcept;
    VkBuffer getIndexBuffer() const noexcept;
    uint32_t getIndexCount() const noexcept;
    VkIndexType getIndexType() const noexcept;
    bool useIndexedDraw() const noexcept;
    uint32_t getVertexCount() const noexcept;
    uint32_t getInstanceCount() const noexcept;

private:
    std::string _vertexShaderPath;
    std::string _fragmentShaderPath;
    std::vector<VkVertexInputBindingDescription> _bindings;
    std::vector<VkVertexInputAttributeDescription> _attributes;
    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    VkDeviceSize _vertexOffset = 0;
    VkBuffer _indexBuffer = VK_NULL_HANDLE;
    uint32_t _indexCount = 0;
    VkIndexType _indexType = VK_INDEX_TYPE_UINT32;
    bool _useIndexedDraw = false;
    uint32_t _vertexCount = 3;
    uint32_t _instanceCount = 1;
};
