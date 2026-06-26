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

        constexpr NodeInputPinDef kVkFramebufferInputs[kVkFramebufferInputPinCount] = {
            {"renderPass", NodeType::VkRenderPass},
            {"pAttachments", NodeType::VkImageView},
        };

        constexpr NodeInputPinDef kVkImageViewInputs[kVkImageViewInputPinCount] = {
            {"image", NodeType::VkImage},
        };

        constexpr NodeInputPinDef kVkSubpassDescriptionInputs[kVkSubpassDescriptionInputPinCount] = {
            {"pColorAttachments", NodeType::VkAttachmentReference},
            {"pDepthStencilAttachment", NodeType::VkAttachmentReference},
        };

        constexpr const char kVkAttachmentLoadOpOptionNames[kVkAttachmentLoadOpOptionCount][32] = {
            "VK_ATTACHMENT_LOAD_OP_LOAD",
            "VK_ATTACHMENT_LOAD_OP_CLEAR",
            "VK_ATTACHMENT_LOAD_OP_DONT_CARE",
        };

        constexpr const char kVkAttachmentStoreOpOptionNames[kVkAttachmentStoreOpOptionCount][36] = {
            "VK_ATTACHMENT_STORE_OP_STORE",
            "VK_ATTACHMENT_STORE_OP_DONT_CARE",
        };

        constexpr const char kVkImageLayoutOptionNames[kVkImageLayoutOptionCount][52] = {
            "VK_IMAGE_LAYOUT_UNDEFINED",
            "VK_IMAGE_LAYOUT_GENERAL",
            "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL",
            "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL",
            "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL",
            "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL",
            "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL",
            "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL",
            "VK_IMAGE_LAYOUT_PREINITIALIZED",
            "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR",
        };

        constexpr const char kVkPipelineBindPointOptionNames[kVkPipelineBindPointOptionCount][40] = {
            "VK_PIPELINE_BIND_POINT_GRAPHICS",
            "VK_PIPELINE_BIND_POINT_COMPUTE",
        };

        constexpr const char kVkImageViewTypeOptionNames[kVkImageViewTypeOptionCount][36] = {
            "VK_IMAGE_VIEW_TYPE_1D",
            "VK_IMAGE_VIEW_TYPE_2D",
            "VK_IMAGE_VIEW_TYPE_3D",
            "VK_IMAGE_VIEW_TYPE_CUBE",
            "VK_IMAGE_VIEW_TYPE_1D_ARRAY",
            "VK_IMAGE_VIEW_TYPE_2D_ARRAY",
            "VK_IMAGE_VIEW_TYPE_CUBE_ARRAY",
        };

        constexpr NodeType kPinLinkTargetNodeTypes[] = {
            NodeType::VkPipeline,
            NodeType::VkRenderPass,
            NodeType::VkPipelineShaderStage,
            NodeType::VkPipelineVertexInputState,
            NodeType::VkPipelineColorBlendState,
            NodeType::VkPipelineColorBlendAttachmentState,
            NodeType::VkPipelineDynamicState,
            NodeType::VkPipelineLayout,
            NodeType::VkDescriptorSetLayout,
            NodeType::VkSubpassDescription,
            NodeType::VkFramebuffer,
            NodeType::VkImageView,
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

        constexpr const char kVkDynamicStateOutputPinLabels[kVkDynamicStateOutputPinCount][40] = {
            "VK_DYNAMIC_STATE_VIEWPORT",
            "VK_DYNAMIC_STATE_SCISSOR",
            "VK_DYNAMIC_STATE_LINE_WIDTH",
            "VK_DYNAMIC_STATE_BLEND_CONSTANTS",
            "VK_DYNAMIC_STATE_DEPTH_BIAS",
        };

        constexpr NodeInputPinDef kVkPipelineVertexInputStateInputs[kVkPipelineVertexInputStateInputPinCount] = {
            {"pVertexBindingDescriptions", NodeType::Vertex, 0},
            {"pVertexAttributeDescriptions", NodeType::Vertex, 1},
        };

        constexpr NodeInputPinDef kVkPipelineColorBlendStateInputs[kVkPipelineColorBlendStateInputPinCount] = {
            {"pAttachments", NodeType::VkPipelineColorBlendAttachmentState},
        };

        constexpr NodeInputPinDef kVkPipelineColorBlendAttachmentStateInputs
            [kVkPipelineColorBlendAttachmentStateInputPinCount] = {
                {"colorWriteMask", NodeType::VkColorWriteMask},
            };

        constexpr NodeInputPinDef kVkPipelineDynamicStateInputs[kVkPipelineDynamicStateInputPinCount] = {
            {"pDynamicStates", NodeType::VkDynamicState},
        };

        constexpr NodeInputPinDef kVkPipelineShaderStageInputs[kVkPipelineShaderStageInputPinCount] = {
            {"module", NodeType::VkShaderModule},
        };

        constexpr NodeInputPinDef kVkPipelineLayoutInputs[kVkPipelineLayoutInputPinCount] = {
            {"pSetLayouts", NodeType::VkDescriptorSetLayout},
        };

        constexpr NodeInputPinDef kVkDescriptorSetLayoutInputs[kVkDescriptorSetLayoutInputPinCount] = {
            {"pBindings", NodeType::VkDescriptorSetLayoutBinding},
        };

        constexpr const char kVkDescriptorTypeOptionNames[kVkDescriptorTypeOptionCount][48] = {
            "VK_DESCRIPTOR_TYPE_SAMPLER",
            "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER",
            "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE",
            "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE",
            "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER",
            "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER",
            "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER",
            "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER",
            "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC",
            "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC",
            "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT",
        };

        constexpr const char kVkShaderStageFlagOptionNames[kVkShaderStageFlagOptionCount][48] = {
            "VK_SHADER_STAGE_VERTEX_BIT",
            "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT",
            "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT",
            "VK_SHADER_STAGE_GEOMETRY_BIT",
            "VK_SHADER_STAGE_FRAGMENT_BIT",
            "VK_SHADER_STAGE_COMPUTE_BIT",
            "VK_SHADER_STAGE_ALL",
        };

        constexpr const char kVertexOutputPinLabels[kVertexOutputPinCount][40] = {
            "pVertexBindingDescriptions",
            "pVertexAttributeDescriptions",
        };

    }  // namespace

    const char kVkPipelineVertexInputStateSType[] =
        "VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO";

    const char kVkPipelineInputAssemblyStateSType[] =
        "VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO";

    const char kVkPipelineShaderStageSType[] = "VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO";

    const char kVkPipelineViewportStateSType[] = "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO";

    const char kVkPipelineRasterizationStateSType[] =
        "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO";

    const char kVkPipelineMultisampleStateSType[] = "VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO";

    const char kVkPipelineDepthStencilStateSType[] =
        "VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO";

    const char kVkPipelineColorBlendStateSType[] = "VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO";

    const char kVkPipelineDynamicStateSType[] = "VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO";

    const char kVkPipelineLayoutSType[] = "VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO";

    const char kVkDescriptorSetLayoutSType[] = "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO";

    const char kVkDescriptorSetLayoutBindingNullSampler[] = "nullptr";

    const char kVkFramebufferSType[] = "VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO";

    const char kVkImageViewSType[] = "VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO";

    const char* nodeTypeName(NodeType type) {
        switch (type) {
            case NodeType::VkPipeline:
                return "VkPipeline";
            case NodeType::VkPipelineShaderStage:
                return "VkPipelineShaderStage";
            case NodeType::VkShaderModule:
                return "VkShaderModule";
            case NodeType::Vertex:
                return "Vertex";
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
            case NodeType::VkDynamicState:
                return "VkDynamicState";
            case NodeType::VkPipelineDynamicState:
                return "VkPipelineDynamicState";
            case NodeType::VkDescriptorSetLayoutBinding:
                return "VkDescriptorSetLayoutBinding";
            case NodeType::VkDescriptorSetLayout:
                return "VkDescriptorSetLayout";
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
            case NodeType::VkAttachmentReference:
                return "VkAttachmentReference";
            case NodeType::VkImage:
                return "VkImage";
            case NodeType::VkImageView:
                return "VkImageView";
            case NodeType::VkFramebuffer:
                return "VkFramebuffer";
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
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (1 + 1 + kVkRenderPassAddAttachmentRowCount + kVkRenderPassFixedInputPinCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineShaderStage) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineShaderStagePrefixParamCount + kVkPipelineShaderStageInputPinCount +
                               kVkPipelineShaderStageSuffixParamCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkShaderModule) {
            return ImVec2(kNodeWidth, kNodeHeaderHeight + kVkShaderModuleParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::Vertex) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (3 + kVertexAddItemRowCount + kVertexOutputPinCount) * kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineVertexInputState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineVertexInputStateParamCount + kVkPipelineVertexInputStateInputPinCount) *
                                  kNodePinRowHeight);
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
        if (type == NodeType::VkDynamicState) {
            return ImVec2(kNodeWidth, kNodeHeaderHeight + kVkDynamicStateOutputPinCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineDynamicState) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineDynamicStateParamCount + kVkPipelineDynamicStateInputPinCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkPipelineLayout) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkPipelineLayoutParamCount + kVkPipelineLayoutInputPinCount) * kNodePinRowHeight);
        }
        if (type == NodeType::VkDescriptorSetLayout) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkDescriptorSetLayoutParamCount + kVkDescriptorSetLayoutInputPinCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkDescriptorSetLayoutBinding) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkDescriptorSetLayoutBindingParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkAttachmentDescription) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkAttachmentDescriptionParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkSubpassDescription) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkSubpassDescriptionParamCount + kVkSubpassDescriptionInputPinCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkSubpassDependency) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkSubpassDependencyParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkAttachmentReference) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight + kVkAttachmentReferenceParamCount * kNodePinRowHeight);
        }
        if (type == NodeType::VkImageView) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkImageViewPrefixParamCount + kVkImageViewInputPinCount +
                               kVkImageViewSuffixParamCount) *
                                  kNodePinRowHeight);
        }
        if (type == NodeType::VkFramebuffer) {
            return ImVec2(kNodeWidth,
                          kNodeHeaderHeight +
                              (kVkFramebufferParamCount + kVkFramebufferInputPinCount) * kNodePinRowHeight);
        }
        return ImVec2(kNodeWidth, kNodeHeaderHeight + kNodeEmptyBodyHeight);
    }

    bool nodeHasOutputPin(NodeType type) {
        return nodeOutputPinCount(type) > 0;
    }

    bool nodeHasInputPins(NodeType type) {
        if (type == NodeType::VkRenderPass) {
            return true;
        }
        return nodeInputPinCount(type) > 0;
    }

    bool nodeInputPinAllowsMultipleLinks(NodeType type, int pinIndex) {
        if (type == NodeType::VkPipeline && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkPipelineColorBlendState && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkPipelineDynamicState && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkPipelineLayout && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkDescriptorSetLayout && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkSubpassDescription && pinIndex == 0) {
            return true;
        }
        if (type == NodeType::VkFramebuffer && pinIndex == 1) {
            return true;
        }
        return false;
    }

    int nodeInputPinCount(NodeType type) {
        if (type == NodeType::VkPipeline) {
            return kVkPipelineInputPinCount;
        }
        if (type == NodeType::VkSubpassDescription) {
            return kVkSubpassDescriptionInputPinCount;
        }
        if (type == NodeType::VkFramebuffer) {
            return kVkFramebufferInputPinCount;
        }
        if (type == NodeType::VkImageView) {
            return kVkImageViewInputPinCount;
        }
        if (type == NodeType::VkPipelineVertexInputState) {
            return kVkPipelineVertexInputStateInputPinCount;
        }
        if (type == NodeType::VkPipelineColorBlendState) {
            return kVkPipelineColorBlendStateInputPinCount;
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState) {
            return kVkPipelineColorBlendAttachmentStateInputPinCount;
        }
        if (type == NodeType::VkPipelineDynamicState) {
            return kVkPipelineDynamicStateInputPinCount;
        }
        if (type == NodeType::VkPipelineLayout) {
            return kVkPipelineLayoutInputPinCount;
        }
        if (type == NodeType::VkDescriptorSetLayout) {
            return kVkDescriptorSetLayoutInputPinCount;
        }
        if (type == NodeType::VkPipelineShaderStage) {
            return kVkPipelineShaderStageInputPinCount;
        }
        return 0;
    }

    int nodeInputPinBodyRow(NodeType type, int pinIndex) {
        if (pinIndex < 0) {
            return -1;
        }
        if (type == NodeType::VkPipeline) {
            return pinIndex;
        }
        if (type == NodeType::VkSubpassDescription) {
            return kVkSubpassDescriptionParamCount + pinIndex;
        }
        if (type == NodeType::VkFramebuffer) {
            return kVkFramebufferParamCount + pinIndex;
        }
        if (type == NodeType::VkImageView) {
            return kVkImageViewPrefixParamCount + pinIndex;
        }
        if (type == NodeType::VkPipelineVertexInputState) {
            return kVkPipelineVertexInputStateParamCount + pinIndex;
        }
        if (type == NodeType::VkPipelineColorBlendState) {
            return kVkPipelineColorBlendStateParamCount + pinIndex;
        }
        if (type == NodeType::VkPipelineColorBlendAttachmentState) {
            return kVkPipelineColorBlendAttachmentStateParamCount + pinIndex;
        }
        if (type == NodeType::VkPipelineDynamicState) {
            return kVkPipelineDynamicStateParamCount + pinIndex;
        }
        if (type == NodeType::VkPipelineLayout) {
            return kVkPipelineLayoutParamCount + pinIndex;
        }
        if (type == NodeType::VkDescriptorSetLayout) {
            return kVkDescriptorSetLayoutParamCount + pinIndex;
        }
        if (type == NodeType::VkPipelineShaderStage) {
            return kVkPipelineShaderStagePrefixParamCount + pinIndex;
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
        if (type == NodeType::VkSubpassDescription) {
            if (index < 0 || index >= kVkSubpassDescriptionInputPinCount) {
                return nullptr;
            }
            return &kVkSubpassDescriptionInputs[index];
        }
        if (type == NodeType::VkFramebuffer) {
            if (index < 0 || index >= kVkFramebufferInputPinCount) {
                return nullptr;
            }
            return &kVkFramebufferInputs[index];
        }
        if (type == NodeType::VkImageView) {
            if (index < 0 || index >= kVkImageViewInputPinCount) {
                return nullptr;
            }
            return &kVkImageViewInputs[index];
        }
        if (type == NodeType::VkRenderPass) {
            return nullptr;
        }
        if (type == NodeType::VkPipelineVertexInputState) {
            if (index < 0 || index >= kVkPipelineVertexInputStateInputPinCount) {
                return nullptr;
            }
            return &kVkPipelineVertexInputStateInputs[index];
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
        if (type == NodeType::VkPipelineDynamicState) {
            if (index < 0 || index >= kVkPipelineDynamicStateInputPinCount) {
                return nullptr;
            }
            return &kVkPipelineDynamicStateInputs[index];
        }
        if (type == NodeType::VkPipelineLayout) {
            if (index < 0 || index >= kVkPipelineLayoutInputPinCount) {
                return nullptr;
            }
            return &kVkPipelineLayoutInputs[index];
        }
        if (type == NodeType::VkDescriptorSetLayout) {
            if (index < 0 || index >= kVkDescriptorSetLayoutInputPinCount) {
                return nullptr;
            }
            return &kVkDescriptorSetLayoutInputs[index];
        }
        if (type == NodeType::VkPipelineShaderStage) {
            if (index < 0 || index >= kVkPipelineShaderStageInputPinCount) {
                return nullptr;
            }
            return &kVkPipelineShaderStageInputs[index];
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
        if (type == NodeType::Vertex) {
            return kVertexOutputPinCount;
        }
        if (type == NodeType::VkDynamicState) {
            return kVkDynamicStateOutputPinCount;
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
        if (type == NodeType::VkDynamicState) {
            if (pinIndex < 0 || pinIndex >= kVkDynamicStateOutputPinCount) {
                return "";
            }
            return kVkDynamicStateOutputPinLabels[pinIndex];
        }
        if (type == NodeType::Vertex) {
            if (pinIndex < 0 || pinIndex >= kVertexOutputPinCount) {
                return "";
            }
            return kVertexOutputPinLabels[pinIndex];
        }
        return nodeTypeName(type);
    }

    bool nodeInputPinAcceptsSource(NodeType inputNodeType, int inputPinIndex, NodeType sourceType,
                                   int sourcePinIndex) {
        const NodeInputPinDef* pinDef = nodeInputPin(inputNodeType, inputPinIndex);
        if (pinDef == nullptr || pinDef->slotType != sourceType) {
            return false;
        }
        if (pinDef->slotSourcePinIndex >= 0 && sourcePinIndex >= 0 &&
            pinDef->slotSourcePinIndex != sourcePinIndex) {
            return false;
        }
        return true;
    }

    NodeType pinLinkTargetNodeTypeForSource(NodeType sourceType) {
        if (sourceType == NodeType::VkAttachmentDescription) {
            return NodeType::VkRenderPass;
        }
        if (sourceType == NodeType::VkImage) {
            return NodeType::VkImageView;
        }
        if (sourceType == NodeType::VkImageView) {
            return NodeType::VkFramebuffer;
        }
        for (NodeType nodeType : kPinLinkTargetNodeTypes) {
            if (nodeInputPinIndexForType(nodeType, sourceType) >= 0) {
                return nodeType;
            }
        }
        return NodeType::VkPipeline;
    }

    int nodeInputPinIndexForType(NodeType nodeType, NodeType slotType, int slotSourcePinIndex) {
        const int pinCount = nodeInputPinCount(nodeType);
        for (int index = 0; index < pinCount; ++index) {
            if (nodeInputPinAcceptsSource(nodeType, index, slotType, slotSourcePinIndex)) {
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

    const char* vkDescriptorTypeOptionName(int index) {
        if (index < 0 || index >= kVkDescriptorTypeOptionCount) {
            return "";
        }
        return kVkDescriptorTypeOptionNames[index];
    }

    const char* vkShaderStageFlagOptionName(int index) {
        if (index < 0 || index >= kVkShaderStageFlagOptionCount) {
            return "";
        }
        return kVkShaderStageFlagOptionNames[index];
    }

    const char* vkAttachmentLoadOpOptionName(int index) {
        if (index < 0 || index >= kVkAttachmentLoadOpOptionCount) {
            return "";
        }
        return kVkAttachmentLoadOpOptionNames[index];
    }

    const char* vkAttachmentStoreOpOptionName(int index) {
        if (index < 0 || index >= kVkAttachmentStoreOpOptionCount) {
            return "";
        }
        return kVkAttachmentStoreOpOptionNames[index];
    }

    const char* vkImageLayoutOptionName(int index) {
        if (index < 0 || index >= kVkImageLayoutOptionCount) {
            return "";
        }
        return kVkImageLayoutOptionNames[index];
    }

    const char* vkPipelineBindPointOptionName(int index) {
        if (index < 0 || index >= kVkPipelineBindPointOptionCount) {
            return "";
        }
        return kVkPipelineBindPointOptionNames[index];
    }

    const char* vkImageViewTypeOptionName(int index) {
        if (index < 0 || index >= kVkImageViewTypeOptionCount) {
            return "";
        }
        return kVkImageViewTypeOptionNames[index];
    }

}  // namespace mat::demo
