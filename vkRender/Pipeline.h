#pragma once

#include <volk.h>
#include <vector>

class Pipeline {
public:
    struct GraphicsCreateInfo {
        VkDevice device = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkExtent2D extent{};
        VkShaderModule vertShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragShaderModule = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkPushConstantRange> pushConstantRanges;
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        bool enableDepthTest = false;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    };

    struct ComputeCreateInfo {
        VkDevice device = VK_NULL_HANDLE;
        VkShaderModule computeShaderModule = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkPushConstantRange> pushConstantRanges;
    };

public:
    // Create graphics pipeline + graphics pipeline layout.
    void initializeGraphics(const GraphicsCreateInfo& info);

    // Create compute pipeline + compute pipeline layout.
    void initializeCompute(const ComputeCreateInfo& info);

    // Destroy graphics pipeline resources only.
    void cleanupGraphics();

    // Destroy compute pipeline resources only.
    void cleanupCompute();

    // Destroy all pipeline resources.
    void cleanup();

    // Bind graphics pipeline to command buffer.
    void bindGraphics(VkCommandBuffer commandBuffer) const;

    // Bind compute pipeline to command buffer.
    void bindCompute(VkCommandBuffer commandBuffer) const;

    VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getGraphicsPipelineLayout() const { return graphicsPipelineLayout; }
    VkPipeline getComputePipeline() const { return computePipeline; }
    VkPipelineLayout getComputePipelineLayout() const { return computePipelineLayout; }

    // Helper for creating/destroying shader modules from SPIR-V words.
    static VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t>& spirvCode);
    static void destroyShaderModule(VkDevice device, VkShaderModule shaderModule);

private:
    void ensureDevice(VkDevice inDevice);
    VkPipelineLayout createPipelineLayout(
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
        const std::vector<VkPushConstantRange>& pushConstantRanges) const;

private:
    VkDevice device = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout graphicsPipelineLayout = VK_NULL_HANDLE;
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
};
