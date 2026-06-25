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

        constexpr const char kVkPrimitiveTopologyOptionNames[kVkPrimitiveTopologyOptionCount][64] = {
            "VK_PRIMITIVE_TOPOLOGY_POINT_LIST",
            "VK_PRIMITIVE_TOPOLOGY_LINE_LIST",
            "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP",
            "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY",
            "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY",
            "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST",
            "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP",
            "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN",
            "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY",
            "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY",
        };

        constexpr const char kVkPolygonModeOptionNames[kVkPolygonModeOptionCount][32] = {
            "VK_POLYGON_MODE_FILL",
            "VK_POLYGON_MODE_LINE",
            "VK_POLYGON_MODE_POINT",
        };

        constexpr const char kVkCullModeOptionNames[kVkCullModeOptionCount][32] = {
            "VK_CULL_MODE_NONE",
            "VK_CULL_MODE_FRONT_BIT",
            "VK_CULL_MODE_BACK_BIT",
            "VK_CULL_MODE_FRONT_AND_BACK",
        };

        constexpr const char kVkFrontFaceOptionNames[kVkFrontFaceOptionCount][40] = {
            "VK_FRONT_FACE_COUNTER_CLOCKWISE",
            "VK_FRONT_FACE_CLOCKWISE",
        };

        constexpr const char kVkSampleCountOptionNames[kVkSampleCountOptionCount][32] = {
            "VK_SAMPLE_COUNT_1_BIT",
            "VK_SAMPLE_COUNT_2_BIT",
            "VK_SAMPLE_COUNT_4_BIT",
            "VK_SAMPLE_COUNT_8_BIT",
            "VK_SAMPLE_COUNT_16_BIT",
            "VK_SAMPLE_COUNT_32_BIT",
            "VK_SAMPLE_COUNT_64_BIT",
        };

        constexpr const char kVkCompareOpOptionNames[kVkCompareOpOptionCount][32] = {
            "VK_COMPARE_OP_NEVER",
            "VK_COMPARE_OP_LESS",
            "VK_COMPARE_OP_EQUAL",
            "VK_COMPARE_OP_LESS_OR_EQUAL",
            "VK_COMPARE_OP_GREATER",
            "VK_COMPARE_OP_NOT_EQUAL",
            "VK_COMPARE_OP_GREATER_OR_EQUAL",
            "VK_COMPARE_OP_ALWAYS",
        };

        constexpr const char kVkLogicOpOptionNames[kVkLogicOpOptionCount][32] = {
            "VK_LOGIC_OP_CLEAR",
            "VK_LOGIC_OP_AND",
            "VK_LOGIC_OP_AND_REVERSE",
            "VK_LOGIC_OP_COPY",
            "VK_LOGIC_OP_AND_INVERTED",
            "VK_LOGIC_OP_NO_OP",
            "VK_LOGIC_OP_XOR",
            "VK_LOGIC_OP_OR",
            "VK_LOGIC_OP_NOR",
            "VK_LOGIC_OP_EQUIVALENT",
            "VK_LOGIC_OP_INVERT",
            "VK_LOGIC_OP_OR_REVERSE",
            "VK_LOGIC_OP_COPY_INVERTED",
            "VK_LOGIC_OP_OR_INVERTED",
            "VK_LOGIC_OP_NAND",
            "VK_LOGIC_OP_SET",
        };

        constexpr const char kVkColorWriteMaskOutputPinLabels[kVkColorWriteMaskOutputPinCount][32] = {
            "VK_COLOR_COMPONENT_R_BIT",
            "VK_COLOR_COMPONENT_G_BIT",
            "VK_COLOR_COMPONENT_B_BIT",
            "VK_COLOR_COMPONENT_A_BIT",
        };

        constexpr NodeInputPinDef kVkPipelineColorBlendStateInputs[kVkPipelineColorBlendStateInputPinCount] = {
            {"pAttachments", NodeType::VkPipelineColorBlendAttachmentState},
        };

        constexpr NodeInputPinDef kVkPipelineColorBlendAttachmentStateInputs
            [kVkPipelineColorBlendAttachmentStateInputPinCount] = {
                {"colorWriteMask", NodeType::VkColorWriteMask},
            };

        constexpr NodeType kPinLinkTargetNodeTypes[] = {
            NodeType::VkPipeline,
            NodeType::VkRenderPass,
            NodeType::VkPipelineColorBlendState,
            NodeType::VkPipelineColorBlendAttachmentState,
        };

    }  // namespace

    const char kVkPipelineInputAssemblyStateSType[] =
        "VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO";

    const char kVkPipelineViewportStateSType[] = "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO";

    const char kVkPipelineRasterizationStateSType[] =
        "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO";

    const char kVkPipelineMultisampleStateSType[] = "VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO";

    const char kVkPipelineDepthStencilStateSType[] =
        "VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO";

    const char kVkPipelineColorBlendStateSType[] = "VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO";

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
            case NodeType::VkPipelineColorBlendAttachmentState:
                return "VkPipelineColorBlendAttachmentState";
            case NodeType::VkColorWriteMask:
                return "VkColorWriteMask";
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
        if (type == NodeType::VkPipelineInputAssemblyState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkPipelineInputAssemblyStateParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineViewportState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkPipelineViewportStateParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineRasterizationState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkPipelineRasterizationStateParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineMultisampleState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkPipelineMultisampleStateParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineDepthStencilState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkPipelineDepthStencilStateParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineColorBlendState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineColorBlendStateParamCount + kVkPipelineColorBlendStateInputPinCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineColorBlendAttachmentStateParamCount +
                               kVkPipelineColorBlendAttachmentStateInputPinCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkColorWriteMask) {
            return ImVec2(kNodeWidth, kNodeHeaderHeight + kVkColorWriteMaskOutputPinCount * kNodePinRowHeight);
        }
        return ImVec2(kNodeWidth, kNodeHeaderHeight + kNodeEmptyBodyHeight);
    }

    bool nodeHasOutputPin(NodeType type) {
        return nodeOutputPinCount(type) > 0;
    }

    bool nodeHasInputPins(NodeType type) {
        return nodeInputPinCount(type) > 0;
    }

    bool nodeInputPinAllowsMultipleLinks(NodeType type, int pinIndex) {
        if (type == NodeType::VkRenderPass) {
            return pinIndex >= 0 && pinIndex < kVkRenderPassInputPinCount;
        }
        if (type == NodeType::VkPipeline && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkPipelineColorBlendState && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState && pinIndex == 0) {
            return true;
        }
        return false;
    }

    int nodeInputPinCount(NodeType type) {
        if (type == NodeType::VkPipeline) {
            return kVkPipelineInputPinCount;
        }
        if (type == NodeType::VkRenderPass) {
            return kVkRenderPassInputPinCount;
        }
        if (type == NodeType::VkPipelineColorBlendState) {
            return kVkPipelineColorBlendStateInputPinCount;
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState) {
            return kVkPipelineColorBlendAttachmentStateInputPinCount;
        }
        return 0;
    }

    int nodeInputPinBodyRow(NodeType type, int pinIndex) {
        if (pinIndex < 0) {
            return -1;
        }
        if (type == NodeType::VkPipeline || type == NodeType::VkRenderPass) {
            return pinIndex;
        }
        if (type == NodeType::VkPipelineColorBlendState) {
            return kVkPipelineColorBlendStateParamCount + pinIndex;
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState) {
            return kVkPipelineColorBlendAttachmentStateParamCount + pinIndex;
        }
        return pinIndex;
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
        if (type == NodeType::VkPipelineColorBlendState) {
            if (index < 0 || index >= kVkPipelineColorBlendStateInputPinCount) {
                return nullptr;
            }
            return &kVkPipelineColorBlendStateInputs[index];
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState) {
            if (index < 0 || index >= kVkPipelineColorBlendAttachmentStateInputPinCount) {
                return nullptr;
            }
            return &kVkPipelineColorBlendAttachmentStateInputs[index];
        }
        return nullptr;
    }

    int nodeOutputPinCount(NodeType type) {
        if (type == NodeType::VkPipeline) {
            return 0;
        }
        if (type == NodeType::VkColorWriteMask) {
            return kVkColorWriteMaskOutputPinCount;
        }
        return 1;
    }

    const char* nodeOutputPinLabel(NodeType type, int pinIndex) {
        if (type == NodeType::VkColorWriteMask) {
            if (pinIndex < 0 || pinIndex >= kVkColorWriteMaskOutputPinCount) {
                return "";
            }
            return kVkColorWriteMaskOutputPinLabels[pinIndex];
        }
        return nodeTypeName(type);
    }

    NodeType pinLinkTargetNodeTypeForSource(NodeType sourceType) {
        for (NodeType nodeType : kPinLinkTargetNodeTypes) {
            if (nodeInputPinIndexForType(nodeType, sourceType) >= 0) {
                return nodeType;
            }
        }
        return NodeType::VkPipeline;
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

    const char* vkPrimitiveTopologyOptionName(int index) {
        if (index < 0 || index >= kVkPrimitiveTopologyOptionCount) {
            return "";
        }
        return kVkPrimitiveTopologyOptionNames[index];
    }

    const char* vkPolygonModeOptionName(int index) {
        if (index < 0 || index >= kVkPolygonModeOptionCount) {
            return "";
        }
        return kVkPolygonModeOptionNames[index];
    }

    const char* vkCullModeOptionName(int index) {
        if (index < 0 || index >= kVkCullModeOptionCount) {
            return "";
        }
        return kVkCullModeOptionNames[index];
    }

    const char* vkFrontFaceOptionName(int index) {
        if (index < 0 || index >= kVkFrontFaceOptionCount) {
            return "";
        }
        return kVkFrontFaceOptionNames[index];
    }

    const char* vkBool32OptionName(bool value) {
        return value ? "VK_TRUE" : "VK_FALSE";
    }

    const char* vkSampleCountOptionName(int index) {
        if (index < 0 || index >= kVkSampleCountOptionCount) {
            return "";
        }
        return kVkSampleCountOptionNames[index];
    }

    const char* vkCompareOpOptionName(int index) {
        if (index < 0 || index >= kVkCompareOpOptionCount) {
            return "";
        }
        return kVkCompareOpOptionNames[index];
    }

    const char* vkLogicOpOptionName(int index) {
        if (index < 0 || index >= kVkLogicOpOptionCount) {
            return "";
        }
        return kVkLogicOpOptionNames[index];
    }

}  // namespace mat::demo
