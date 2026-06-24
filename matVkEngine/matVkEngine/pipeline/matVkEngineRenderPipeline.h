#pragma once

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineRenderPipeline {
    public:
        VkEngineRenderPipeline();
        ~VkEngineRenderPipeline();

        void setShaderStages(std::vector<VkPipelineShaderStageCreateInfo> stages);

        void setVertexInputInfo(const VkPipelineVertexInputStateCreateInfo& info);

        void setInputAssembly(const VkPipelineInputAssemblyStateCreateInfo& info);

        void setViewportState(const VkPipelineViewportStateCreateInfo& info);

        void setRasterizer(const VkPipelineRasterizationStateCreateInfo& info);
        
        void setMultisampling(const VkPipelineMultisampleStateCreateInfo& info);

        void setDepthStencil(const VkPipelineDepthStencilStateCreateInfo& info);

        void setColorBlending(const VkPipelineColorBlendStateCreateInfo& info);

        void setDynamicState(const VkPipelineDynamicStateCreateInfo& info);

        void setPipelineLayout(const VkPipelineLayout& layout);

        void setRenderPass(VkRenderPass pass, uint32_t passId);

        void create(VkDevice device);

    private:
        VkEngineRenderPipeline(const VkEngineRenderPipeline&) = delete;
        VkEngineRenderPipeline(VkEngineRenderPipeline&&) = delete;
        VkEngineRenderPipeline& operator=(const VkEngineRenderPipeline&) = delete;
        VkEngineRenderPipeline& operator=(VkEngineRenderPipeline&&) = delete;

        VkGraphicsPipelineCreateInfo _info{};
        VkPipeline _pipeline = VK_NULL_HANDLE;
    };

};  // namespace mat