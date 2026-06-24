#include "graph/GraphTypes.h"

namespace mat::demo {

    namespace {

        constexpr VkPipelineInputPinDef kVkPipelineInputs[kVkPipelineInputPinCount] = {
            {"VkPipelineShaderStage", NodeType::VkPipelineShaderStage},
            {"VkPipelineVertexInputState", NodeType::VkPipelineVertexInputState},
            {"VkPipelineInputAssemblyState", NodeType::VkPipelineInputAssemblyState},
            {"VkPipelineViewportState", NodeType::VkPipelineViewportState},
            {"VkPipelineRasterizationState", NodeType::VkPipelineRasterizationState},
            {"VkPipelineMultisampleState", NodeType::VkPipelineMultisampleState},
            {"VkPipelineDepthStencilState", NodeType::VkPipelineDepthStencilState},
            {"VkPipelineColorBlendState", NodeType::VkPipelineColorBlendState},
            {"VkPipelineDynamicState", NodeType::VkPipelineDynamicState},
            {"VkPipelineLayout", NodeType::VkPipelineLayout},
            {"VkRenderPass", NodeType::VkRenderPass},
        };

    }  // namespace

    const char* nodeTypeName(NodeType type) {
        switch (type) {
            case NodeType::VkPipeline:
                return "VkPipeline";
            case NodeType::VkPipelineShaderStage:
                return "VkPipelineShaderStage";
            case NodeType::VkPipelineVertexInputState:
                return "VkPipelineVertexInputState";
            case NodeType::VkPipelineInputAssemblyState:
                return "VkPipelineInputAssemblyState";
            case NodeType::VkPipelineViewportState:
                return "VkPipelineViewportState";
            case NodeType::VkPipelineRasterizationState:
                return "VkPipelineRasterizationState";
            case NodeType::VkPipelineMultisampleState:
                return "VkPipelineMultisampleState";
            case NodeType::VkPipelineDepthStencilState:
                return "VkPipelineDepthStencilState";
            case NodeType::VkPipelineColorBlendState:
                return "VkPipelineColorBlendState";
            case NodeType::VkPipelineDynamicState:
                return "VkPipelineDynamicState";
            case NodeType::VkPipelineLayout:
                return "VkPipelineLayout";
            case NodeType::VkRenderPass:
                return "VkRenderPass";
        }
        return "Unknown";
    }

    ImVec2 nodeWorldSize(NodeType type) {
        if (type == NodeType::VkPipeline) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineInputPinCount + kVkPipelineIndexRowCount) * kNodePinRowHeight);
        }
        return ImVec2(kNodeWidth, kNodeHeaderHeight + kNodeEmptyBodyHeight);
    }

    bool nodeHasOutputPin(NodeType type) {
        return type != NodeType::VkPipeline;
    }

    const VkPipelineInputPinDef* vkPipelineInputPin(int index) {
        if (index < 0 || index >= kVkPipelineInputPinCount) {
            return nullptr;
        }
        return &kVkPipelineInputs[index];
    }

}  // namespace mat::demo
