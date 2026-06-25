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
        VkAttachmentDescription,
        VkSubpassDescription,
        VkSubpassDependency,
    };

    constexpr float kNodeWidth = 280.f;
    constexpr float kNodeHeaderHeight = 30.f;
    constexpr float kNodePinRowHeight = 22.f;
    constexpr float kNodeEmptyBodyHeight = 24.f;
    constexpr int kVkPipelineInputPinCount = 11;
    constexpr int kVkRenderPassInputPinCount = 3;
    constexpr int kVkPipelineIndexRowCount = 1;

    struct NodeInputPinDef {
        const char* label;
        NodeType slotType;
    };

    const char* nodeTypeName(NodeType type);
    ImVec2 nodeWorldSize(NodeType type);
    bool nodeHasOutputPin(NodeType type);
    bool nodeHasInputPins(NodeType type);
    bool nodeInputPinAllowsMultipleLinks(NodeType type);
    int nodeInputPinCount(NodeType type);
    const NodeInputPinDef* nodeInputPin(NodeType type, int index);
    int nodeInputPinIndexForType(NodeType nodeType, NodeType slotType);

}  // namespace mat::demo
