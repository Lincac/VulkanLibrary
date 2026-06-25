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
        VkPipelineColorBlendAttachmentState,
        VkColorWriteMask,
        VkDynamicState,
        VkPipelineDynamicState,
        VkDescriptorSetLayoutBinding,
        VkDescriptorSetLayout,
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
    constexpr int kVkPipelineInputAssemblyStateParamCount = 3;
    constexpr int kVkPipelineViewportStateParamCount = 3;
    constexpr int kVkPipelineRasterizationStateParamCount = 8;
    constexpr int kVkPipelineMultisampleStateParamCount = 3;
    constexpr int kVkPipelineDepthStencilStateParamCount = 6;
    constexpr int kVkPipelineColorBlendStateParamCount = 7;
    constexpr int kVkPipelineColorBlendStateInputPinCount = 1;
    constexpr int kVkPipelineColorBlendAttachmentStateParamCount = 1;
    constexpr int kVkPipelineColorBlendAttachmentStateInputPinCount = 1;
    constexpr int kVkColorWriteMaskOutputPinCount = 4;
    constexpr int kVkDynamicStateOutputPinCount = 5;
    constexpr int kVkPipelineDynamicStateParamCount = 1;
    constexpr int kVkPipelineDynamicStateInputPinCount = 1;
    constexpr int kVkPipelineLayoutParamCount = 1;
    constexpr int kVkPipelineLayoutInputPinCount = 1;
    constexpr int kVkDescriptorSetLayoutParamCount = 1;
    constexpr int kVkDescriptorSetLayoutInputPinCount = 1;
    constexpr int kVkDescriptorSetLayoutBindingParamCount = 5;
    constexpr int kVkDescriptorTypeOptionCount = 11;
    constexpr int kVkShaderStageFlagOptionCount = 7;
    constexpr int kVkPrimitiveTopologyOptionCount = 10;
    constexpr int kVkPolygonModeOptionCount = 3;
    constexpr int kVkCullModeOptionCount = 4;
    constexpr int kVkFrontFaceOptionCount = 2;
    constexpr int kVkSampleCountOptionCount = 7;
    constexpr int kVkCompareOpOptionCount = 8;
    constexpr int kVkLogicOpOptionCount = 16;

    extern const char kVkPipelineInputAssemblyStateSType[];
    extern const char kVkPipelineViewportStateSType[];
    extern const char kVkPipelineRasterizationStateSType[];
    extern const char kVkPipelineMultisampleStateSType[];
    extern const char kVkPipelineDepthStencilStateSType[];
    extern const char kVkPipelineColorBlendStateSType[];
    extern const char kVkPipelineDynamicStateSType[];
    extern const char kVkPipelineLayoutSType[];
    extern const char kVkDescriptorSetLayoutSType[];
    extern const char kVkDescriptorSetLayoutBindingNullSampler[];

    struct NodeInputPinDef {
        const char* label;
        NodeType slotType;
    };

    const char* nodeTypeName(NodeType type);
    ImVec2 nodeWorldSize(NodeType type);
    bool nodeHasOutputPin(NodeType type);
    bool nodeHasInputPins(NodeType type);
    bool nodeInputPinAllowsMultipleLinks(NodeType type, int pinIndex);
    int nodeInputPinCount(NodeType type);
    int nodeInputPinBodyRow(NodeType type, int pinIndex);
    const NodeInputPinDef* nodeInputPin(NodeType type, int index);
    int nodeInputPinIndexForType(NodeType nodeType, NodeType slotType);
    int nodeOutputPinCount(NodeType type);
    const char* nodeOutputPinLabel(NodeType type, int pinIndex);
    NodeType pinLinkTargetNodeTypeForSource(NodeType sourceType);
    const char* vkPrimitiveTopologyOptionName(int index);
    const char* vkPolygonModeOptionName(int index);
    const char* vkCullModeOptionName(int index);
    const char* vkFrontFaceOptionName(int index);
    const char* vkBool32OptionName(bool value);
    const char* vkSampleCountOptionName(int index);
    const char* vkCompareOpOptionName(int index);
    const char* vkLogicOpOptionName(int index);
    const char* vkDescriptorTypeOptionName(int index);
    const char* vkShaderStageFlagOptionName(int index);

}  // namespace mat::demo
