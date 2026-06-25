#include "graph/GraphTypes.h"

namespace mat::demo {

    namespace {

        constexpr NodeInputPinDef kVkPipelineInputs[kVkPipelineInputPinCount] = {
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

        constexpr NodeInputPinDef kVkRenderPassInputs[kVkRenderPassInputPinCount] = {
            {"VkAttachmentDescription", NodeType::VkAttachmentDescription},
            {"VkSubpassDescription", NodeType::VkSubpassDescription},
            {"VkSubpassDependency", NodeType::VkSubpassDependency},
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
            case NodeType::VkAttachmentDescription:
                return "VkAttachmentDescription";
            case NodeType::VkSubpassDescription:
                return "VkSubpassDescription";
            case NodeType::VkSubpassDependency:
                return "VkSubpassDependency";
        }
        return "Unknown";
    }

    ImVec2 nodeWorldSize(NodeType type) {
        if (type == NodeType::VkPipeline) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineInputPinCount + kVkPipelineIndexRowCount) * kNodePinRowHeight);
        }
        if (type == NodeType::VkRenderPass) {
            return ImVec2(kNodeWidth, kNodeHeaderHeight + kVkRenderPassInputPinCount * kNodePinRowHeight);
        }
        return ImVec2(kNodeWidth, kNodeHeaderHeight + kNodeEmptyBodyHeight);
    }

    bool nodeHasOutputPin(NodeType type) {
        return type != NodeType::VkPipeline;
    }

    bool nodeHasInputPins(NodeType type) {
        return type == NodeType::VkPipeline || type == NodeType::VkRenderPass;
    }

    bool nodeInputPinAllowsMultipleLinks(NodeType type) {
        return type == NodeType::VkRenderPass;
    }

    int nodeInputPinCount(NodeType type) {
        if (type == NodeType::VkPipeline) {
            return kVkPipelineInputPinCount;
        }
        if (type == NodeType::VkRenderPass) {
            return kVkRenderPassInputPinCount;
        }
        return 0;
    }

    const NodeInputPinDef* nodeInputPin(NodeType type, int index) {
        if (type == NodeType::VkPipeline) {
            if (index < 0 || index >= kVkPipelineInputPinCount) {
                return nullptr;
            }
            return &kVkPipelineInputs[index];
        }
        if (type == NodeType::VkRenderPass) {
            if (index < 0 || index >= kVkRenderPassInputPinCount) {
                return nullptr;
            }
            return &kVkRenderPassInputs[index];
        }
        return nullptr;
    }

    int nodeInputPinIndexForType(NodeType nodeType, NodeType slotType) {
        const int pinCount = nodeInputPinCount(nodeType);
        for (int index = 0; index < pinCount; ++index) {
            const NodeInputPinDef* pinDef = nodeInputPin(nodeType, index);
            if (pinDef != nullptr && pinDef->slotType == slotType) {
                return index;
            }
        }
        return -1;
    }

}  // namespace mat::demo
