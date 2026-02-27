#pragma once

#include <volk.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class FrameGraph {
public:
    using ResourceId = uint32_t;
    using PassId = uint32_t;

    enum class ResourceType : uint8_t {
        Image,
        Buffer
    };

    enum class AccessType : uint8_t {
        Read,
        Write,
        ReadWrite
    };

    struct ImageDesc {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t layers = 1;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        bool imported = false;
        bool exported = false;
        bool transient = true;
    };

    struct BufferDesc {
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bool imported = false;
        bool exported = false;
        bool transient = true;
    };

    struct ResourceUsage {
        ResourceId resource = 0;
        AccessType access = AccessType::Read;
        VkPipelineStageFlags2 stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        VkAccessFlags2 accessMask = 0;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct Barrier {
        ResourceId resource = 0;
        PassId srcPass = 0;
        PassId dstPass = 0;
        VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 srcAccessMask = 0;
        VkAccessFlags2 dstAccessMask = 0;
        VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct ResourceLifetime {
        ResourceId resource = 0;
        int32_t firstUse = -1;
        int32_t lastUse = -1;
        uint64_t estimatedBytes = 0;
    };

    struct AliasAllocation {
        ResourceId resource = 0;
        uint32_t slot = 0;
        uint64_t slotSizeBytes = 0;
    };

    struct CompileResult {
        bool valid = false;
        std::vector<PassId> executionOrder;
        std::vector<PassId> culledPasses;
        std::vector<Barrier> barriers;
        std::vector<ResourceLifetime> lifetimes;
        std::vector<AliasAllocation> aliasAllocations;
    };

    struct PassBuilder {
        PassId pass = 0;
        std::string name;
        bool sideEffect = false;
        bool enabled = true;
        std::vector<ResourceUsage> reads;
        std::vector<ResourceUsage> writes;
        std::function<void(VkCommandBuffer)> callback;
    };

public:
    ResourceId createImage(const std::string& name, const ImageDesc& desc);
    ResourceId createBuffer(const std::string& name, const BufferDesc& desc);

    PassId addPass(const std::string& name, bool sideEffect = false);
    void setPassEnabled(PassId pass, bool enabled);
    void setPassCallback(PassId pass, std::function<void(VkCommandBuffer)> callback);
    void readResource(PassId pass, const ResourceUsage& usage);
    void writeResource(PassId pass, const ResourceUsage& usage);

    void markResourceExported(ResourceId resource, bool exported = true);
    void markResourceImported(ResourceId resource, bool imported = true);
    void bindImage(ResourceId resource, VkImage image, VkImageSubresourceRange subresourceRange);
    void bindBuffer(
        ResourceId resource,
        VkBuffer buffer,
        VkDeviceSize offset = 0,
        VkDeviceSize size = VK_WHOLE_SIZE);

    // Build execution order, culling info, dependency barriers and aliasing plan.
    void compile();

    // Execute compiled passes in order.
    void execute(VkCommandBuffer commandBuffer) const;

    void reset();

    const CompileResult& getCompileResult() const { return compileResult; }

private:
    struct ResourceNode {
        std::string name;
        ResourceType type = ResourceType::Image;
        ImageDesc image{};
        BufferDesc buffer{};
    };

    struct RuntimeResource {
        VkImage image = VK_NULL_HANDLE;
        VkImageSubresourceRange imageRange{};
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize bufferOffset = 0;
        VkDeviceSize bufferSize = VK_WHOLE_SIZE;
    };

private:
    uint64_t estimateResourceBytes(const ResourceNode& resource) const;
    static bool hasWriteHazard(AccessType lhs, AccessType rhs);
    static bool isImageType(ResourceType type);
    void emitBarriersForPass(VkCommandBuffer commandBuffer, PassId pass) const;

private:
    std::vector<ResourceNode> resources;
    std::vector<RuntimeResource> runtimeResources;
    std::vector<PassBuilder> passes;
    CompileResult compileResult;
};
