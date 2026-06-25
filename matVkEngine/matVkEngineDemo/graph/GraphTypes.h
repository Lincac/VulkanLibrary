#pragma once

#include <imgui.h>

namespace mat::demo {

    enum class NodeType {
        VkPipeline,
        VkPipelineShaderStage,
        VkPipelineVertexInputState,
        VkPipelineInputAssemblyState,
        VkPipelineViewportState,
        VkPipelineRasterizationState,
        VkPipelineMultisampleState,
        VkPipelineDepthStencilState,
        VkPipelineColorBlendState,
        VkPipelineDynamicState,
        VkPipelineLayout,
        VkRenderPass,
    };

    constexpr float kNodeWidth = 280.f;
    constexpr float kNodeHeaderHeight = 30.f;
    constexpr float kNodePinRowHeight = 22.f;
    constexpr float kNodeEmptyBodyHeight = 24.f;
    constexpr int kVkPipelineInputPinCount = 11;
    constexpr int kVkPipelineIndexRowCount = 1;

    struct VkPipelineInputPinDef {
        const char* label;
        NodeType slotType;
    };

    const char* nodeTypeName(NodeType type);
    ImVec2 nodeWorldSize(NodeType type);
    bool nodeHasOutputPin(NodeType type);
    const VkPipelineInputPinDef* vkPipelineInputPin(int index);
    int vkPipelineInputPinIndexForType(NodeType type);

}  // namespace mat::demo
